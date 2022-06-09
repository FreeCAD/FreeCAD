<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="hr" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../draftobjects/wire.py" line="49"/>
      <source>The vertices of the wire</source>
      <translation>Vrhovi žice</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="53"/>
      <source>If the wire is closed or not</source>
      <translation>Dali je žica zatvorena ili ne</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="57"/>
      <source>The base object is the wire, it's formed from 2 objects</source>
      <translation>Osnovni objekt je žica, sastoji se od 2 objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="61"/>
      <source>The tool object is the wire, it's formed from 2 objects</source>
      <translation>Osnovni objekt je žica, sastoji se od 2 objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="65"/>
      <source>The start point of this line</source>
      <translation>Početna točka ove linije</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="69"/>
      <source>The end point of this line</source>
      <translation>Krajnja točka ove linije</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="73"/>
      <source>The length of this line</source>
      <translation>Dužina ove linije</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="52"/>
      <location filename="../../draftobjects/polygon.py" line="60"/>
      <location filename="../../draftobjects/wire.py" line="77"/>
      <source>Radius to use to fillet the corners</source>
      <translation>Radijus koji se koristi za zaobljivanje uglova</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="55"/>
      <location filename="../../draftobjects/polygon.py" line="64"/>
      <location filename="../../draftobjects/wire.py" line="81"/>
      <source>Size of the chamfer to give to the corners</source>
      <translation>Veličina oluka obljivanje uglova</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="85"/>
      <source>Create a face if this object is closed</source>
      <translation>Stvaranje lice ako je ovaj objekt zatvoren</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="89"/>
      <source>The number of subdivisions of each edge</source>
      <translation>Broj podjeljaka svakog ruba</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="67"/>
      <location filename="../../draftobjects/circle.py" line="62"/>
      <location filename="../../draftobjects/polygon.py" line="72"/>
      <location filename="../../draftobjects/bspline.py" line="57"/>
      <location filename="../../draftobjects/bezcurve.py" line="70"/>
      <location filename="../../draftobjects/wire.py" line="93"/>
      <source>The area of this object</source>
      <translation>Područje ovog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="65"/>
      <source>The position of the tip of the leader line.
This point can be decorated with an arrow or another symbol.</source>
      <translation>Položaj vrha vodeće linije.
Ova točka može biti ukrašena strelicom ili drugim simbolom.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="79"/>
      <source>Object, and optionally subelement, whose properties will be displayed
as 'Text', depending on 'Label Type'.

'Target' won't be used if 'Label Type' is set to 'Custom'.</source>
      <translation>Objekt i po izboru pod-element, čija će svojstva biti prikazana
kao 'Tekst', ovisno o 'Vrsti oznake'.
'Cilj' se neće koristiti ako je 'Vrsta oznake' postavljena 'Prilagođeno'.
</translation>
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
      <translation>Popis točaka koje definiraju vodeću liniju; obično popis od tri točke.

Prva točka treba biti pozicija teksta, odnosno 'Položaj',
a zadnja točka treba biti vrh linije, odnosno 'Odredišna točka'.
Srednja točka se izračunava automatski ovisno o odabranom
'Pravi smjer' i vrijednost 'Ravna udaljenost' i znak.

Ako je 'Pravi smjer' postavljeno na 'Prilagođeno', svojstvo 'Točke'.
može se postaviti kao lista proizvoljnih točaka.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="124"/>
      <source>The direction of the straight segment of the leader line.

If 'Custom' is chosen, the points of the leader can be specified by
assigning a custom list to the 'Points' attribute.</source>
      <translation>Smjer ravnog segmenta vodeće linije.

Ako je odabrano 'Prilagođeno', točke opisne linija mogu se odrediti
dodjeljivanjem prilagođenog popisa atributu 'Točke'.

</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="142"/>
      <source>The length of the straight segment of the leader line.

This is an oriented distance; if it is negative, the line will be drawn
to the left or below the 'Text', otherwise to the right or above it,
depending on the value of 'Straight Direction'.</source>
      <translation>Duljina ravnog segmenta vodeće linije.

Ovo je orijentirana udaljenost; ako je negativan, crta će se povući
lijevo ili ispod 'Tekst', inače desno ili iznad njega,
ovisno o vrijednosti 'Ravni smjer'.

</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="156"/>
      <source>The placement of the 'Text' element in 3D space</source>
      <translation>Postavljanje elementa 'Tekst' u 3D prostor</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="166"/>
      <source>The text to display when 'Label Type' is set to 'Custom'</source>
      <translation>Tekst koji se prikazuje kada je vrsta oznake postavljena na 'prilagođeno'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="181"/>
      <source>The text displayed by this label.

This property is read-only, as the final text depends on 'Label Type',
and the object defined in 'Target'.
The 'Custom Text' is displayed only if 'Label Type' is set to 'Custom'.</source>
      <translation>Tekst prikazan ovom oznakom.

Ovo svojstvo je samo za čitanje, budući da konačni tekst ovisi o 'Vrsta oznake',
i objekt definiran u 'Odredište'.
'Prilagođen tekst' prikazuje se samo ako je 'Vrsta oznaka' postavljena na 'Prilagođeno'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="216"/>
      <source>The type of information displayed by this label.

If 'Custom' is chosen, the contents of 'Custom Text' will be used.
For other types, the string will be calculated automatically from the object defined in 'Target'.
'Tag' and 'Material' only work for objects that have these properties, like Arch objects.

For 'Position', 'Length', and 'Area' these properties will be extracted from the main object in 'Target',
or from the subelement 'VertexN', 'EdgeN', or 'FaceN', respectively, if it is specified.</source>
      <translation>Vrsta informacija koje prikazuje ova oznaka.

Ako je odabrano 'Prilagođeno', koristit će se sadržaj 'Prilagođen tekst'.
Za druge vrste, slova će se automatski izračunati iz objekta definiranog u 'Odredište'.
'Opis' i 'Materijal' rade samo za objekte koji imaju ta svojstva, kao što su Luk objekti.

Za 'Položaj', 'Dužinu' i 'Površinu' ova svojstva će se izdvojiti iz glavnog objekta u 'Odredište',
ili iz pod-elementa 'VrhN', 'RubN', odnosno 'LiceN', ako je navedeno.
</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="46"/>
      <source>The base object used by this object</source>
      <translation>Osnovni objekt korišten kod ovog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="49"/>
      <source>The PAT file used by this object</source>
      <translation>PAT datoteka korištena kod ovog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="52"/>
      <source>The pattern name used by this object</source>
      <translation>Ime uzorka korištenog kod ovog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="55"/>
      <source>The pattern scale used by this object</source>
      <translation>Skaliranje uzorka korištenog kod ovog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="58"/>
      <source>The pattern rotation used by this object</source>
      <translation>Rotacija uzorka korištenog kod ovog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="61"/>
      <source>If set to False, hatch is applied as is to the faces, without translation (this might give wrong results for non-XY faces)</source>
      <translation>Ako je postavljeno na Lažno, šrafiranje se primjenjuje kao što jest na lica, bez pomjeranja (ovo bi moglo dati pogrešne rezultate za lica koja su izvan XY)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="66"/>
      <source>The linked object</source>
      <translation>Povezani objekt</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="73"/>
      <source>Projection direction</source>
      <translation>Smjer projekcije</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="80"/>
      <source>The width of the lines inside this object</source>
      <translation>Duljina linija unutar objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="88"/>
      <source>The size of the texts inside this object</source>
      <translation>Veličina tekstova unutar ovog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="96"/>
      <source>The spacing between lines of text</source>
      <translation>Razmaci između linija teksta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="103"/>
      <source>The color of the projected objects</source>
      <translation>Boja projiciranog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="110"/>
      <source>Shape Fill Style</source>
      <translation>Stil ispune oblika</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="118"/>
      <source>Line Style</source>
      <translation>Stil crte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="127"/>
      <source>If checked, source objects are displayed regardless of being visible in the 3D model</source>
      <translation>Ako je označeno, izvorni objekti prikazuju se bez obzira jesu li vidljivi u 3D modelu</translation>
    </message>
    <message>
      <location filename="../../draftobjects/circle.py" line="45"/>
      <source>Start angle of the arc</source>
      <translation>Početni kut luka</translation>
    </message>
    <message>
      <location filename="../../draftobjects/circle.py" line="50"/>
      <source>End angle of the arc (for a full circle, 
                give it same value as First Angle)</source>
      <translation type="unfinished">End angle of the arc (for a full circle, 
                give it same value as First Angle)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/circle.py" line="54"/>
      <source>Radius of the circle</source>
      <translation>Polumjer kruga</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="58"/>
      <location filename="../../draftobjects/circle.py" line="58"/>
      <location filename="../../draftobjects/polygon.py" line="68"/>
      <location filename="../../draftobjects/ellipse.py" line="58"/>
      <source>Create a face</source>
      <translation>Napravite naličje</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="46"/>
      <source>Text string</source>
      <translation>Slova riječi</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="49"/>
      <source>Font file name</source>
      <translation>Ime datoteke pisma</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="52"/>
      <source>Height of text</source>
      <translation>Visina teksta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="55"/>
      <source>Inter-character spacing</source>
      <translation>Unutrašnji razmak između znakova</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="58"/>
      <source>Fill letters with faces</source>
      <translation>Ispunite slova sa površinama</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="86"/>
      <source>The base object that will be duplicated.</source>
      <translation>Osnovni objekt koji će biti dupliciran.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="93"/>
      <location filename="../../draftobjects/patharray.py" line="177"/>
      <source>The object along which the copies will be distributed. It must contain 'Edges'.</source>
      <translation>Primjenjuje trenutni stil definiran na alatnoj traci (širina linije i boje) na odabrane objekte i grupe.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="100"/>
      <source>Number of copies to create.</source>
      <translation>Broj kopija koje će biti napravljene.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="107"/>
      <source>Rotation factor of the twisted array.</source>
      <translation>Faktor rotacije prepletene matrice.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="320"/>
      <location filename="../../draftobjects/pathtwistedarray.py" line="114"/>
      <location filename="../../draftobjects/pointarray.py" line="112"/>
      <location filename="../../draftobjects/patharray.py" line="208"/>
      <source>Show the individual array elements (only for Link arrays)</source>
      <translation>Prikaži pojedinačne elemente niza (samo za vezane nizove)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="83"/>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="82"/>
      <source>General scaling factor that affects the annotation consistently
because it scales the text, and the line decorations, if any,
in the same proportion.</source>
      <translation>Općeniti faktor skaliranja utječe na anotiranje kozistentno
jer skalira tekst, i udeđivanja linija, ako postoje,
u jednakoj mjeri.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="93"/>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="100"/>
      <source>Annotation style to apply to this object.
When using a saved style some of the view properties will become read-only;
they will only be editable by changing the style through the 'Annotation style editor' tool.</source>
      <translation>Stil anotiranja koji će se primjeniti na ovaj objekt.
Kada se koristi spremljeni stil neka od svojstava pregleda će se moći samo čitati;
ona će se moći mijenjati samo promjenom stila pomoću 'Urednika za Anotacijske Stilove'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draftlink.py" line="99"/>
      <source>Force sync pattern placements even when array elements are expanded</source>
      <translation>Prisilno sinkroniziranje položaja uzoraka čak i kada su elementi matrice prošireni

</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draftlink.py" line="112"/>
      <source>Show the individual array elements</source>
      <translation>Prikaži pojedinačne elemente matrice</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="46"/>
      <source>The objects included in this clone</source>
      <translation>Objekti uljučeni u ovaj klon</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="51"/>
      <source>The scale factor of this clone</source>
      <translation>Faktor promjene veličine ovog klona</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="57"/>
      <source>If Clones includes several objects,
set True for fusion or False for compound</source>
      <translation>Ako Klonovi uključuju nekoliko objekata,
označi spajanje ili odznači složeni</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="46"/>
      <source>The points of the B-spline</source>
      <translation>Točke B-krivulje</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="50"/>
      <source>If the B-spline is closed or not</source>
      <translation>Dali je B-krivulja zatvorena ili ne</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="54"/>
      <source>Create a face if this spline is closed</source>
      <translation>Stvaranje lica, ako je ova krivulja zatvorena</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="67"/>
      <source>Parameterization factor</source>
      <translation>Faktor Parametriranja</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="57"/>
      <source>The base object this 2D view must represent</source>
      <translation>Osnovni objekt 2D prikaza mora predstavljati</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="62"/>
      <source>The projection vector of this object</source>
      <translation>Projekcijski vektor ovog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="68"/>
      <source>The way the viewed object must be projected</source>
      <translation>Način na koji objekt pregleda mora biti projiciran</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="75"/>
      <source>The indices of the faces to be projected in Individual Faces mode</source>
      <translation>Indeksi lica će biti projicirani u načinu rada pojedinih lica</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="80"/>
      <source>Show hidden lines</source>
      <translation>Prikaži skrivene linije</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="86"/>
      <source>Fuse wall and structure objects of same type and material</source>
      <translation>Spoji zid i objekte strukture ako su istog tipa i materijala</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="91"/>
      <source>Tessellate Ellipses and B-splines into line segments</source>
      <translation>Razlaganje elipsa i B-krivulja u linijske segmente</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="98"/>
      <source>For Cutlines and Cutfaces modes, 
                    this leaves the faces at the cut location</source>
      <translation type="unfinished">For Cutlines and Cutfaces modes, 
                    this leaves the faces at the cut location</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="105"/>
      <source>Length of line segments if tessellating Ellipses or B-splines 
                    into line segments</source>
      <translation type="unfinished">Length of line segments if tessellating Ellipses or B-splines 
                    into line segments</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="111"/>
      <source>If this is True, this object will include only visible objects</source>
      <translation>Ako je ovo Istinito, ovaj će objekt uključivati ​​samo vidljive objekte

</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="117"/>
      <source>A list of exclusion points. Any edge touching any of those points will not be drawn.</source>
      <translation>Popis točaka isključenja. Bilo koji rub koji dodiruje bilo koju od tih točaka neće biti nacrtan.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="122"/>
      <source>If this is True, only solid geometry is handled. This overrides the base object's Only Solids property</source>
      <translation>Ako je ovo Istinito, obrađuje se samo čvrsta tijela geometrija. Ovo nadjačava samo (Samo čvrsto tijelo) svojstvo od osnovnog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="127"/>
      <source>If this is True, the contents are clipped to the borders of the section plane, if applicable. This overrides the base object's Clip property</source>
      <translation> Ako je Istinito, sadržaj se izrezuje na granice ravnine presjeka, ako je primjenjivo. To nadjačava svojstvo (Isječak) osnovnog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="132"/>
      <source>This object will be recomputed only if this is True.</source>
      <translation>Ovaj će se objekt ponovno izračunati samo ako je ovo Istinito.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="45"/>
      <source>X Location</source>
      <translation>X Pozicija</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="48"/>
      <source>Y Location</source>
      <translation>Y Pozicija</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="51"/>
      <source>Z Location</source>
      <translation>Z Pozicija</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="46"/>
      <source>Length of the rectangle</source>
      <translation>Duljina pravokutnika</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="49"/>
      <source>Height of the rectangle</source>
      <translation>Visina pravokutnika</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="61"/>
      <source>Horizontal subdivisions of this rectangle</source>
      <translation>Horizontalne podjele ovog pravokutnika</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="64"/>
      <source>Vertical subdivisions of this rectangle</source>
      <translation>Vertikalne podjele ovog pravokutnika</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="43"/>
      <source>Linked faces</source>
      <translation>Povezana naličja</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="46"/>
      <source>Specifies if splitter lines must be removed</source>
      <translation>Određuje dali razmakne linije moraju biti uklonjene</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="49"/>
      <source>An optional extrusion value to be applied to all faces</source>
      <translation>Opcionalna vrijednost izguravanja za primjenu na svim naličjima</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="52"/>
      <source>An optional offset value to be applied to all faces</source>
      <translation>Opcionalna vrijednost izguravanja za primjenu na svim naličjima</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="55"/>
      <source>This specifies if the shapes sew</source>
      <translation>Određuje dali se oblici sastavljaju</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="58"/>
      <source>The area of the faces of this Facebinder</source>
      <translation>Površina naličja ovog Facebindera</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="48"/>
      <source>Number of faces</source>
      <translation>Broj naličja</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="52"/>
      <source>Radius of the control circle</source>
      <translation>Radijus kontrolnog kruga</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="56"/>
      <source>How the polygon must be drawn from the control circle</source>
      <translation>Kako se poligon mora izvući iz kontrolnog kruga</translation>
    </message>
    <message>
      <location filename="../../draftobjects/block.py" line="43"/>
      <source>The components of this block</source>
      <translation>Sastavni dijelovi ovog bloka</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="47"/>
      <source>The start point of this line.</source>
      <translation>Početna točka ove linije.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="55"/>
      <source>The end point of this line.</source>
      <translation>Krajnja točka ove linije.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="63"/>
      <source>The length of this line.</source>
      <translation>Dužina ove linije.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="71"/>
      <source>Radius to use to fillet the corner.</source>
      <translation>Radijus za zaobljivanje ugla.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wpproxy.py" line="42"/>
      <source>The placement of this object</source>
      <translation>polozaj objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/layer.py" line="59"/>
      <source>The objects that are part of this layer</source>
      <translation>Objekti koji su dio od ovog sloja</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="137"/>
      <source>The normal direction of the text of the dimension</source>
      <translation>Smjer normale teksta dimenzije</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="150"/>
      <source>The object measured by this dimension object</source>
      <translation>Objekt mjeren ovim mjernim objektom</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="167"/>
      <source>The object, and specific subelements of it,
that this dimension object is measuring.

There are various possibilities:
- An object, and one of its edges.
- An object, and two of its vertices.
- An arc object, and its edge.</source>
      <translation>Objekt i njegovi specifični podelementi,
kad se ovaj objekt dimenzija mjeri.

Postoje razne mogućnosti:
- Predmet i jedan od njegovih rubova.
- Objekt i dva njegova vrha.
- Lučni objekt i njegov rub.</translation>
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
      <translation>Točka kroz koju će mjerna linija, ili njena ekstrapolacija, proći.

- Za linearne mjere, ovo svojstvo kontrolira koliko blizu mjerenog objekta
je mjerna linija.
- Za radijalne mjere, ovo kontrolira smjer mjerne linije
koja prikazuje mjereni radijus ili promjer.
- Za kutne mjere, ovo kontrolira radijus jernog luka
koji prikazuje mjereni kut.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="233"/>
      <source>Starting point of the dimension line.

If it is a radius dimension it will be the center of the arc.
If it is a diameter dimension it will be a point that lies on the arc.</source>
      <translation>Početna točka mjerne linije.

Ako se mjeri radijus ona će biti centar luka.
Ako se mjeri promjer ona će biti tička koja leži na luku.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="247"/>
      <source>Ending point of the dimension line.

If it is a radius or diameter dimension
it will be a point that lies on the arc.</source>
      <translation>Krajnja točka mjerne linije.

Ako se mjeri radijus ili promjer
biti će točka koja leži na luku.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="259"/>
      <source>The direction of the dimension line.
If this remains '(0,0,0)', the direction will be calculated automatically.</source>
      <translation>Smjer mjerne linije.
Ako ovo ostane '(0,0,0)', smjer će se izračunati automatski.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="276"/>
      <source>The value of the measurement.

This property is read-only because the value is calculated
from the 'Start' and 'End' properties.

If the 'Linked Geometry' is an arc or circle, this 'Distance'
is the radius or diameter, depending on the 'Diameter' property.</source>
      <translation>Rezultat mjerenja.

Ovo svojstvo je samo za čitanje jer se vrijednost računa
iz 'Početnih' i 'Krajnjih' svojstava.

Ako je 'Povezana Geometrija' luk ili krug, ova 'Udaljenost'
je radijus ili promjer, ovisno o svojstvu 'Promjer'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="287"/>
      <source>When measuring circular arcs, it determines whether to display
the radius or the diameter value</source>
      <translation>Kada se mjere kružni lukovi, ono određuje dali će se prikazati
radijus ili promjer</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="507"/>
      <source>Starting angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
      <translation>Početni kut mjerne linije ( kružnog luka).
Luk se iscrtava smjeru suprotnom od kazaljke na satu.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="518"/>
      <source>Ending angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
      <translation>Krajni kut mjerne linije (kružni luk).
Luk se iscrtava u smjeru suprotnom od kazaljke na satu.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="533"/>
      <source>The center point of the dimension line, which is a circular arc.

This is normally the point where two line segments, or their extensions
intersect, resulting in the measured 'Angle' between them.</source>
      <translation>Središnja točka mjerne linije, koja je kružni luk.

Ovo je uobičajeno točka gdje se dva segmenta linije, ili njihova proširenja
sijeku, rezultirajući izmjerenim 'Kutom' među njima.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="547"/>
      <source>The value of the measurement.

This property is read-only because the value is calculated from
the 'First Angle' and 'Last Angle' properties.</source>
      <translation>Rezultat mjerenja.

Ovo svojstvo je samo za čitanje jer se vrijednost računa iz
svojstava 'Prvi Kut' i 'Posljednji Kut'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="45"/>
      <source>Start angle of the elliptical arc</source>
      <translation>Početni kut eliptičnog luka</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="49"/>
      <source>End angle of the elliptical arc 

                (for a full circle, give it same value as First Angle)</source>
      <translation type="unfinished">End angle of the elliptical arc 

                (for a full circle, give it same value as First Angle)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="52"/>
      <source>Minor radius of the ellipse</source>
      <translation>Sporedni polumjer elipse</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="55"/>
      <source>Major radius of the ellipse</source>
      <translation>Glavni polumjer elipse</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="61"/>
      <source>Area of this object</source>
      <translation>Područje ovog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/text.py" line="54"/>
      <source>The placement of the base point of the first line</source>
      <translation>Postavljanje osnovne točke prve linije</translation>
    </message>
    <message>
      <location filename="../../draftobjects/text.py" line="66"/>
      <source>The text displayed by this object.
It is a list of strings; each element in the list will be displayed in its own line.</source>
      <translation>Tekst prikazan ovim objektom.
To je niz tekstova; avaki element u listi će biti prikazan u vlastitoj liniji.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="82"/>
      <location filename="../../draftobjects/patharray.py" line="169"/>
      <source>The base object that will be duplicated</source>
      <translation>Osnovni objekt koji će biti dupliciran</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="192"/>
      <source>List of connected edges in the 'Path Object'.
If these are present, the copies will be created along these subelements only.
Leave this property empty to create copies along the entire 'Path Object'.</source>
      <translation>Popis povezanih rubova u 'Objekt puta'.
Ako su oni prisutni, kopije će se kreirati samo uzduž ovih pod-elementa.
Ostavite ovo svojstvo praznim da biste stvorili kopije duž cijelog 'Objekt puta'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="200"/>
      <source>Number of copies to create</source>
      <translation>Broj kopija koje će biti napravljene</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="219"/>
      <source>Additional translation that will be applied to each copy.
This is useful to adjust for the difference between shape centre and shape reference point.</source>
      <translation>Dodatno pomjeranje koje će se primijeniti na svaki primjerak.
Ovo je korisno za podešavanje razlike između središta oblika i referentne točke oblika.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="227"/>
      <source>Alignment vector for 'Tangent' mode</source>
      <translation>Vektor poravnavanja za 'Tangencijalno' način</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="235"/>
      <source>Force use of 'Vertical Vector' as local Z direction when using 'Original' or 'Tangent' alignment mode</source>
      <translation>Forsiraj korištenje 'Vertikalnog Vektora' kao lokalni Z smjer kada se koristi 'Originalni' ili 'Tangencijalni' način poravnanja</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="243"/>
      <source>Direction of the local Z axis when 'Force Vertical' is true</source>
      <translation>Smjer lokalne Z osi kada je 'Forsiraj Vertikalni' označen</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="251"/>
      <source>Method to orient the copies along the path.
- Original: X is curve tangent, Y is normal, and Z is the cross product.
- Frenet: aligns the object following the local coordinate system along the path.
- Tangent: similar to 'Original' but the local X axis is pre-aligned to 'Tangent Vector'.

To get better results with 'Original' or 'Tangent' you may have to set 'Force Vertical' to true.</source>
      <translation>Način orijentacije kopija duž puta.
- Originalni: X je tangenta na krivulju, Y je noramla, a Z je vektorski produkt.
- Frenet: poravnava objekt prateći lokalne koordinatni sustav duž puta.
- Tangencijalni: slično 'Originalnom' ali lokalna X os je unaprijed poravnata s 'Tangencijalnim Vektorom'.

Kako bi dobili bolje rezultate s 'Originalnim' ili 'Tangencijalnim' moguće je da će te morati označiti 'Forsiraj Vertikani'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="262"/>
      <source>Orient the copies along the path depending on the 'Align Mode'.
Otherwise the copies will have the same orientation as the original Base object.</source>
      <translation>Orijentiraj kopije duž puta ovisno o 'Načinu Poravnanja'.
Inače kopije će imati istu orijetaciju kao i originalni Osnovni objekt.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="46"/>
      <source>The points of the Bezier curve</source>
      <translation>Točke Bezijerove krivulje</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="50"/>
      <source>The degree of the Bezier function</source>
      <translation>Stupanj Bezier funkcije</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="54"/>
      <source>Continuity</source>
      <translation>Kontinuitet</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="58"/>
      <source>If the Bezier curve should be closed or not</source>
      <translation>Da li Bezijerova krivulja treba biti zatvorena ili ne</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="62"/>
      <source>Create a face if this curve is closed</source>
      <translation>Stvaranje lica, ako je ova krivulja zatvorena</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="66"/>
      <source>The length of this object</source>
      <translation>Dužina ovog objekta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="98"/>
      <source>The type of array to create.
- Ortho: places the copies in the direction of the global X, Y, Z axes.
- Polar: places the copies along a circular arc, up to a specified angle, and with certain orientation defined by a center and an axis.
- Circular: places the copies in concentric circular layers around the base object.</source>
      <translation>Vrsta niza za izradu.
 - Ortogonalni: stavlja kopije u smjerovima globalnih X, Y, Z osi.
 - Polarni: stavlja kopije duž krućnog luka, do određenog kuta, u određenoj orijentaciji defniranonoj s centrom i osi.
 - Kručni: stavlja kopije u koncentrične kružne slojeve oko osnovnog objekta.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="109"/>
      <source>Specifies if the copies should be fused together if they touch each other (slower)</source>
      <translation>Određuje dali kopije trebaju srasti ako se dodiruju (sporije)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="122"/>
      <source>Number of copies in X direction</source>
      <translation>Broj kopije u X smjeru</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="131"/>
      <source>Number of copies in Y direction</source>
      <translation>Broj kopija u Y smjeru</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="140"/>
      <source>Number of copies in Z direction</source>
      <translation>Broj kopija u Z smjeru</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="150"/>
      <source>Distance and orientation of intervals in X direction</source>
      <translation>Razmak i orijentacija intervala u X smjeru</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="160"/>
      <source>Distance and orientation of intervals in Y direction</source>
      <translation>Razmak i orijentacija intervala u Y smjeru</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="170"/>
      <source>Distance and orientation of intervals in Z direction</source>
      <translation>Razmak i orijentacija intervala u Z smjeru</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="185"/>
      <source>The axis direction around which the elements in a polar or a circular array will be created</source>
      <translation>Smjer osi oko koje će se elementi polarnom ili kružnom nizu izraditi</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="196"/>
      <source>Center point for polar and circular arrays.
The 'Axis' passes through this point.</source>
      <translation>Centralna točka polarnog ili kružnog niza.
'Os' prolazi kroz tu točku.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="215"/>
      <source>The axis object that overrides the value of 'Axis' and 'Center', for example, a datum line.
Its placement, position and rotation, will be used when creating polar and circular arrays.
Leave this property empty to be able to set 'Axis' and 'Center' manually.</source>
      <translation>Objekt osi koji prepisuje vrijednost 'Osi' i 'Centra', na primjer, datum linija.
Njegovo postavljanje, pozicija i orijentacija če se koristiti kada se kreiraju polarni i kružni nizovi.
Ostavite ovo svojstvo prazno kako bi ste mogli postaviti 'Os' i 'Centar' ručno.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="228"/>
      <source>Number of copies in the polar direction</source>
      <translation>Broj kopija u polarnom smjeru</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="238"/>
      <source>Distance and orientation of intervals in 'Axis' direction</source>
      <translation>Razmak i orijentacija intervala u smjeru osi</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="247"/>
      <source>Angle to cover with copies</source>
      <translation>Kut za pokrivanje sa kopijama</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="260"/>
      <source>Distance between circular layers</source>
      <translation>Udaljenost između kružnih slojeva</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="270"/>
      <source>Distance between copies in the same circular layer</source>
      <translation>Udaljenost između kopija u istom kružnom sloju</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="280"/>
      <source>Number of circular layers. The 'Base' object counts as one layer.</source>
      <translation>Broj kružnih slojeva. Osnovni objekt se broji kao jedan sloj.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="291"/>
      <source>A parameter that determines how many symmetry planes the circular array will have.</source>
      <translation>Parametar koji određuje koliko će ravnina simetrije imati kružni niz.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="309"/>
      <source>Total number of elements in the array.
This property is read-only, as the number depends on the parameters of the array.</source>
      <translation>Ukupan broj elemenata u nizu.
Ovo svojstvo je samo za čitanje, jer broj ovisi o svojstvima niza.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="79"/>
      <source>Base object that will be duplicated</source>
      <translation>Osnovni objekt koji će biti dupliciran</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="87"/>
      <source>Object containing points used to distribute the base object, for example, a sketch or a Part compound.
The sketch or compound must contain at least one explicit point or vertex object.</source>
      <translation>Objekt koji sadrži točke koje se koriste za distribuciju osnovnog objekta, na primjer, skica ili sastavni dio.
Skica ili sastavni dio mora sadržavati barem jednu eksplicitnu točku ili vršni objekt.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="95"/>
      <source>Total number of elements in the array.
This property is read-only, as the number depends on the points contained within 'Point Object'.</source>
      <translation>Ukupan broj elemenata u nizu.
Ovo svojstvo je samo za čitanje, budući da broj ovisi o točkama sadržanim unutar 'Point Object'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="104"/>
      <location filename="../../draftobjects/pointarray.py" line="140"/>
      <source>Additional placement, shift and rotation, that will be applied to each copy</source>
      <translation>Dodatni položaj, pomak i rotacija, koji će se primijeniti na svaku kopiju</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="60"/>
      <location filename="../../draftviewproviders/view_label.py" line="74"/>
      <source>The size of the text</source>
      <translation>Veličina teksta</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="69"/>
      <location filename="../../draftviewproviders/view_label.py" line="83"/>
      <source>The font of the text</source>
      <translation>Vrsta pisma teksta</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="78"/>
      <location filename="../../draftviewproviders/view_label.py" line="92"/>
      <location filename="../../draftviewproviders/view_label.py" line="119"/>
      <source>The vertical alignment of the text</source>
      <translation>Okomito poravnanje teksta</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="87"/>
      <location filename="../../draftviewproviders/view_label.py" line="102"/>
      <source>Text color</source>
      <translation>Boja teksta</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="95"/>
      <location filename="../../draftviewproviders/view_label.py" line="128"/>
      <source>Line spacing (relative to font size)</source>
      <translation>Razmak između redaka (u odnosu na veličinu fonta)</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="111"/>
      <source>The maximum number of characters on each line of the text box</source>
      <translation>Maksimalni broj znakova u svakom retku tekstnog okvira</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="139"/>
      <source>The size of the arrow</source>
      <translation>Veličina strelice</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="148"/>
      <source>The type of arrow of this label</source>
      <translation>Vrsta strelice za ovu oznaku</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="159"/>
      <source>The type of frame around the text of this object</source>
      <translation>Vrsta okvira oko teksta ovog objekta</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="168"/>
      <source>Display a leader line or not</source>
      <translation>Prikaži ili sakrij linije vodilice</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="115"/>
      <location filename="../../draftviewproviders/view_label.py" line="177"/>
      <source>Line width</source>
      <translation>Širina linije</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="122"/>
      <location filename="../../draftviewproviders/view_label.py" line="186"/>
      <source>Line color</source>
      <translation>Boja linije</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="137"/>
      <source>Font name</source>
      <translation>Ime pisma</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="146"/>
      <source>Font size</source>
      <translation>Veličina Pisma</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="155"/>
      <source>Spacing between text and dimension line</source>
      <translation>Razmak između teksta i mjerne linije</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="164"/>
      <source>Rotate the dimension text 180 degrees</source>
      <translation>Rotiraj tekst mjerenja za 180 stupnjeva</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="174"/>
      <source>Text Position.
Leave '(0,0,0)' for automatic position</source>
      <translation>Pozicija teksta.
Ostavite '(0,0,0)' za automatsko pozicioniranje</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="185"/>
      <source>Text override.
Write '$dim' so that it is replaced by the dimension length.</source>
      <translation>Prijepis teksta.
Upišite '$dim' kako bi se zamijenilo s mjerenom duljinom.</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="196"/>
      <source>The number of decimals to show</source>
      <translation>Broj decimalnih mjesta za prikaz</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="205"/>
      <source>Show the unit suffix</source>
      <translation>Pokaži dodatak mjerne jedinice (unit suffix)</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="216"/>
      <source>A unit to express the measurement.
Leave blank for system default.
Use 'arch' to force US arch notation</source>
      <translation>Jedinica za izražavanje mjerenja.
Ostavite prazno za zadane postavke sustava.
Upotrijebite 'Luk' za prisilno označavanje US luka</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="230"/>
      <source>Arrow size</source>
      <translation>Veličina strelice</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="239"/>
      <source>Arrow type</source>
      <translation>Vrsta strelice</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="249"/>
      <source>Rotate the dimension arrows 180 degrees</source>
      <translation>Rotirajte strelice mjerenja za 180 stupnjeva</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="260"/>
      <source>The distance the dimension line is extended
past the extension lines</source>
      <translation>Duljina mjerne linije je proširena
izvan linija protezanja</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="269"/>
      <source>Length of the extension lines</source>
      <translation>Dužina linija produžetaka</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="279"/>
      <source>Length of the extension line
beyond the dimension line</source>
      <translation>Duljina linije protezanja
izvan mjerne linije</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="288"/>
      <source>Shows the dimension line and arrows</source>
      <translation>Prikazuje mjernu liniju i strelice</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="67"/>
      <source>If it is true, the objects contained within this layer will adopt the line color of the layer</source>
      <translation>Ako je označeno, objekti unutar ovog sloja će pokupiti boju linija sloja</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="78"/>
      <source>If it is true, the objects contained within this layer will adopt the shape color of the layer</source>
      <translation>Ako je označeno, objekti unutar ovog sloja će pokupiti boju oblika sloja</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="89"/>
      <source>If it is true, the print color will be used when objects in this layer are placed on a TechDraw page</source>
      <translation>Ako je označeno, boja prinatanja će se koristiti kada se objekti u ovom sloju postave na stranicu TehničkoCrtanje</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="103"/>
      <source>The line color of the objects contained within this layer</source>
      <translation>Boja linija objekata sadržanih u ovom sloju</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="117"/>
      <source>The shape color of the objects contained within this layer</source>
      <translation>Boja oblika objekata sadržanih u ovom sloju</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="131"/>
      <source>The line width of the objects contained within this layer</source>
      <translation>Širina linija objekata sadržanih u ovom sloju</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="143"/>
      <source>The draw style of the objects contained within this layer</source>
      <translation>Stil crtanja objekata u ovom sloju</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="154"/>
      <source>The transparency of the objects contained within this layer</source>
      <translation>Prozirnost objekata sadržanih u ovom sloju</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="165"/>
      <source>The line color of the objects contained within this layer, when used on a TechDraw page</source>
      <translation>Boja linija objekata sadržanih u ovom sloju kada se koristi stranica za TehničkoCrtanje</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_base.py" line="106"/>
      <source>Defines an SVG pattern.</source>
      <translation type="unfinished">Defines an SVG pattern.</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_base.py" line="116"/>
      <source>Defines the size of the SVG pattern.</source>
      <translation type="unfinished">Defines the size of the SVG pattern.</translation>
    </message>
  </context>
  <context>
    <name>Dialog</name>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="14"/>
      <source>Annotation Styles Editor</source>
      <translation>Uređivač stilova napomena</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="20"/>
      <source>Style name</source>
      <translation>Ime stila</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="38"/>
      <source>The name of your style. Existing style names can be edited.</source>
      <translation>Ime vašeg stila. Postojeća imena stilova mogu se uređivati.

</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="50"/>
      <source>Add new...</source>
      <translation>Dodaj novo...</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="79"/>
      <source>Renames the selected style</source>
      <translation>Preimenuje odabrani stil</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="82"/>
      <source>Rename</source>
      <translation>Preimenuj</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="110"/>
      <source>Deletes the selected style</source>
      <translation>Briše odabrani stil</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="113"/>
      <source>Delete</source>
      <translation>Izbriši</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="120"/>
      <source>Import styles from json file</source>
      <translation>Uvezite stilove iz json datoteke

</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="130"/>
      <source>Export styles to json file</source>
      <translation>Izvezite stilove u json datoteku

</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="164"/>
      <source>Text</source>
      <translation>Tekst</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="170"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="192"/>
      <source>The font to use for texts and dimensions</source>
      <translation>Pismo (font) koji se koristi za tekst i dimenzije</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="173"/>
      <source>Font name</source>
      <translation>Ime pisma</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="199"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="209"/>
      <source>Font size in the system units</source>
      <translation>Veličina pisma u jedinicama sustava</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="202"/>
      <source>Font size</source>
      <translation>Veličina Pisma</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="219"/>
      <source>Line spacing in system units</source>
      <translation>Razmak linija u jedinicama sustava</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="222"/>
      <source>Line spacing</source>
      <translation>Razmak između redova</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="239"/>
      <source>Units</source>
      <translation>Mjerne jedinice</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="245"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="255"/>
      <source>A multiplier factor that affects the size of texts and markers</source>
      <translation>Faktor množenja koji utječe na veličinu teksta i markera</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="248"/>
      <source>Scale multiplier</source>
      <translation>Multiplikator skale</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="268"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="278"/>
      <source>If it is checked it will show the unit next to the dimension value</source>
      <translation>Ako je označen, prikazat će se jedinica pored vrijednosti dimenzije</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="271"/>
      <source>Show unit</source>
      <translation>Prikaži jedinice mjere</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="291"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="301"/>
      <source>Specify a valid length unit like mm, m, in, ft, to force displaying the dimension value in this unit</source>
      <translation>Navedite važeću jedinicu duljine kao što su mm, m, in, ft, da biste prisilili na prikaz vrijednosti dimenzija u ovoj jedinici

</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="294"/>
      <source>Unit override</source>
      <translation>Mijenja jedinicu mjere</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="308"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="318"/>
      <source>The number of decimals to show for dimension values</source>
      <translation>Broj decimala koji će se prikazati za vrijednosti dimenzija

</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="311"/>
      <source>Decimals</source>
      <translation>Decimalnih mjesta</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="331"/>
      <source>Line and arrows</source>
      <translation>Linije i strelice</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="337"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="347"/>
      <source>If it is checked it will display the dimension line</source>
      <translation>Ako je označen, prikazat će  liniju dimenzije</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="340"/>
      <source>Show lines</source>
      <translation>Prikaži linije</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="363"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="373"/>
      <source>The width of the dimension lines</source>
      <translation>Širina linija dimenzije</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="366"/>
      <source>Line width</source>
      <translation>Širina linije</translation>
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
      <translation>Boja linija dimenzije, strelica i teksta</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="389"/>
      <source>Line / text color</source>
      <translation>Boja linije / teksta</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="410"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="432"/>
      <source>The type of arrows or markers to use at the end of dimension lines</source>
      <translation>Vrsta strelica ili markera koje treba koristiti na kraju dimenzijskih linija

</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="413"/>
      <source>Arrow type</source>
      <translation>Vrsta strelice</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="436"/>
      <source>Dot</source>
      <translation>Točka</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="441"/>
      <source>Circle</source>
      <translation>Krug</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="446"/>
      <source>Arrow</source>
      <translation>Strelica</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="451"/>
      <source>Tick</source>
      <translation>Okomita crtica</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="456"/>
      <source>Tick-2</source>
      <translation>Okomita crtica-2</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="464"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="474"/>
      <source>The size of the dimension arrows or markers in system units</source>
      <translation>Veličina strelica ili markera dimenzija u jedinicama sustava

</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="467"/>
      <source>Arrow size</source>
      <translation>Veličina strelice</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="484"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="494"/>
      <source>The distance that the dimension line is additionally extended</source>
      <translation>Udaljenost koju linija dodatno produžuje

</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="487"/>
      <source>Dimension overshoot</source>
      <translation>Prekoračenje dimenzije</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="504"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="514"/>
      <source>The length of the extension lines</source>
      <translation>Duljina linija produžetka</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="507"/>
      <source>Extension lines</source>
      <translation>Linije protezanja</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="524"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="534"/>
      <source>The distance that the extension lines are additionally extended beyond the dimension line</source>
      <translation>Udaljenost koju produžne crte dodatno produžuju izvan dimenzijske crte

</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="527"/>
      <source>Extension overshoot</source>
      <translation>Prekoračenje protezanja</translation>
    </message>
  </context>
  <context>
    <name>Draft</name>
    <message>
      <location filename="../../importDXF.py" line="129"/>
      <source>Download of dxf libraries failed.
Please install the dxf Library addon manually
from menu Tools -&gt; Addon Manager</source>
      <translation>Preuzimanje dxf biblioteke nije uspjelo.
Molimo vas ručno instalirati dodatak dxf biblioteke
iz izbornika Alati-&gt; Addon Manager</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="133"/>
      <location filename="../../InitGui.py" line="134"/>
      <location filename="../../InitGui.py" line="135"/>
      <location filename="../../InitGui.py" line="136"/>
      <location filename="../../InitGui.py" line="137"/>
      <source>Draft</source>
      <translation>Skica</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="179"/>
      <location filename="../../InitGui.py" line="180"/>
      <location filename="../../InitGui.py" line="181"/>
      <location filename="../../InitGui.py" line="182"/>
      <source>Import-Export</source>
      <translation>Uvoz / izvoz</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="248"/>
      <source>Toggles Grid On/Off</source>
      <translation>Rešetka prebaci Uključeno/Isključeno</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="265"/>
      <source>Object snapping</source>
      <translation>Privlačenje objekta</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="297"/>
      <source>Toggles Visual Aid Dimensions On/Off</source>
      <translation>Vizualne pomoćne dimenzije Uključi/Isključi</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="316"/>
      <source>Toggles Ortho On/Off</source>
      <translation>Okomito -  Uključeno/Isključeno</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="334"/>
      <source>Toggles Constrain to Working Plane On/Off</source>
      <translation>Ograničiti na radnu ravninu Uključi/Isključi</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="106"/>
      <source>_BSpline.createGeometry: Closed with same first/last Point. Geometry not updated.</source>
      <translation>_BSpline.createGeometry: zatvoreno s istom prvom/zadnjom točkom. Geometrija se ne ažurira.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="244"/>
      <location filename="../../draftobjects/pointarray.py" line="306"/>
      <source>Point object doesn't have a discrete point, it cannot be used for an array.</source>
      <translation>Točkasti objekt nema diskretnu točku, ne može se koristiti za matricu.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="88"/>
      <location filename="../../draftguitools/gui_lineslope.py" line="91"/>
      <source>Slope</source>
      <translation>Nagib</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="91"/>
      <source>Clone</source>
      <translation>Klon</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="49"/>
      <source>You must choose a base object before using this command</source>
      <translation>Prije korištenja ove naredbe morate odabrati osnovni objekt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="89"/>
      <source>Delete original objects</source>
      <translation>Obriši originalne objekte</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="95"/>
      <source>Create chamfer</source>
      <translation>Napravite žlijeb</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="274"/>
      <source>Save style</source>
      <translation>Spremanje stilova</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="275"/>
      <source>Name of this new style:</source>
      <translation>Naziv ovog novog stila:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="282"/>
      <source>Warning</source>
      <translation>Upozorenje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="283"/>
      <source>Name exists. Overwrite?</source>
      <translation>Ime postoji. Prepisati?</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="318"/>
      <source>Error: json module not found. Unable to save style</source>
      <translation>Pogreška: json modul nije pronađen. Nije moguće spremiti stil</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="329"/>
      <source>Offset direction is not defined. Please move the mouse on either side of the object first to indicate a direction</source>
      <translation>Smjer pomaka nije definiran. Prvo pomaknite miš s obje strane objekta kako biste naznačili smjer</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="144"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="156"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="157"/>
      <source>True</source>
      <translation>Točno</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="148"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="160"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="159"/>
      <source>False</source>
      <translation>Netočno</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="151"/>
      <source>Scale</source>
      <translation>Skaliraj</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="152"/>
      <source>X factor</source>
      <translation>X faktor</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="153"/>
      <source>Y factor</source>
      <translation>Y faktor</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="154"/>
      <source>Z factor</source>
      <translation>Z faktor</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="155"/>
      <source>Uniform scaling</source>
      <translation>Jedinstveno skaliranje</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="156"/>
      <source>Working plane orientation</source>
      <translation>Orijentacija radne ravnine</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="157"/>
      <source>Copy</source>
      <translation>Kopiraj</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="158"/>
      <source>Modify subelements</source>
      <translation>Izmjene pod-elemenata</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="159"/>
      <source>Pick from/to points</source>
      <translation>Pokupi od/do točaka</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="160"/>
      <source>Create a clone</source>
      <translation>Stvori klon</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_wpproxy.py" line="102"/>
      <source>Writing camera position</source>
      <translation>Zapiši položaj kamere</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_wpproxy.py" line="119"/>
      <source>Writing objects shown/hidden state</source>
      <translation>Zapiši stanje objekata prikazan/skriven</translation>
    </message>
  </context>
  <context>
    <name>DraftCircularArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="26"/>
      <source>Circular array</source>
      <translation>Kružna matrica</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(Držač mjesta za ikonu)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="53"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="63"/>
      <source>Distance from one layer of objects to the next layer of objects.</source>
      <translation>Udaljenost jednog sloja objekata do sljedećeg sloja objekata.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="56"/>
      <source>Radial distance</source>
      <translation>Radijalna udaljenost</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="76"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="87"/>
      <source>Distance from one element in one ring of the array to the next element in the same ring.
It cannot be zero.</source>
      <translation>Udaljenost od jednog elementa u jednom prstenu matrice do sljedećeg elementa u istom prstenu.
Ovo ne može biti nula.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="80"/>
      <source>Tangential distance</source>
      <translation>Tangencijalna udaljenost</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="101"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="112"/>
      <source>Number of circular layers or rings to create, including a copy of the original object.
It must be at least 2.</source>
      <translation>Broj kružnih slojeva ili prstenova za stvaranje, uključujući kopiju izvornog objekta.
Mora biti najmanje 2.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="105"/>
      <source>Number of circular layers</source>
      <translation>Broj kružnih slojeva</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="126"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="136"/>
      <source>The number of symmetry lines in the circular array.</source>
      <translation>Broj linija simetrije u kružnoj matrici.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="129"/>
      <source>Symmetry</source>
      <translation>Simetrija</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="151"/>
      <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
      <translation>Koordinate točke kroz koju prolazi os rotacije.
Promijenite smjer same osi u uređivaču svojstava.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="155"/>
      <source>Center of rotation</source>
      <translation>Centar rotacije</translation>
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
      <translation>Ponovno postavite koordinate središta rotacije.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="228"/>
      <source>Reset point</source>
      <translation>Obnovi točku</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="240"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Ako je označeno, rezultirajući objekti u matrici bit će spojeni ako se dodiruju.
Ovo djeluje samo ako je "Povezana matrica" isključen.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="244"/>
      <source>Fuse</source>
      <translation>Spoji</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="251"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Ako je označeno, rezultirajući objekt bit će "Povezana matrica" umjesto uobičajenog polja.
Povezana matrica efikasnija je pri stvaranju više kopija, ali ne može ih se spojiti zajedno.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="255"/>
      <source>Link array</source>
      <translation>Povezana matrica</translation>
    </message>
  </context>
  <context>
    <name>DraftOrthoArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="26"/>
      <source>Orthogonal array</source>
      <translation>Ortogonalna matrica</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(Držač mjesta za ikonu)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="51"/>
      <source>Number of elements in the array in the specified direction, including a copy of the original object.
The number must be at least 1 in each direction.</source>
      <translation>Broj elemenata u matrici u navedenom smjeru, uključujući kopiju izvornog objekta.
Broj mora biti najmanje 1 u svakom smjeru.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="55"/>
      <source>Number of elements</source>
      <translation>Broj elemenata</translation>
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
      <translation>Udaljenost između elemenata u X smjeru.
Obično je potrebna samo vrijednost X; ostale dvije vrijednosti mogu dati dodatni pomak u njihovim pravcima.
Negativne vrijednosti rezultirat će kopijom proizvedenom u negativnom smjeru.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="124"/>
      <source>X intervals</source>
      <translation>X intervali</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="197"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="288"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="379"/>
      <source>Reset the distances.</source>
      <translation>Ponovno postavljajte udaljenosti.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="200"/>
      <source>Reset X</source>
      <translation>Resetiraj X</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="210"/>
      <source>Distance between the elements in the Y direction.
Normally, only the Y value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Y intervali</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="215"/>
      <source>Y intervals</source>
      <translation>Y intervali</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="291"/>
      <source>Reset Y</source>
      <translation>Resetiraj Y</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="301"/>
      <source>Distance between the elements in the Z direction.
Normally, only the Z value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Udaljenost između elemenata u smjeru Z.
Obično je potrebna samo vrijednost Z; ostale dvije vrijednosti mogu dati dodatni pomak u njihovim pravcima.
Negativne vrijednosti rezultirat će kopijom proizvedenom u negativnom smjeru.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="306"/>
      <source>Z intervals</source>
      <translation>Z intervali</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="382"/>
      <source>Reset Z</source>
      <translation>Resetiraj  Z</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="394"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Ako je označeno, rezultirajući objekti u matrici bit će spojeni ako se dodiruju.
Ovo djeluje samo ako je "Povezana matrica" isključen.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="398"/>
      <source>Fuse</source>
      <translation>Spoji</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="405"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Ako je označeno, rezultirajući objekt bit će "Povezana matrica" umjesto uobičajenog polja.
Povezana matrica efikasnija je pri stvaranju više kopija, ali ne može ih se spojiti zajedno.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="409"/>
      <source>Link array</source>
      <translation>Povezana matrica</translation>
    </message>
  </context>
  <context>
    <name>DraftPolarArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="26"/>
      <source>Polar array</source>
      <translation>Polarna matrica</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(Držač mjesta za ikonu)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="53"/>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="65"/>
      <source>Sweeping angle of the polar distribution.
A negative angle produces a polar pattern in the opposite direction.
The maximum absolute value is 360 degrees.</source>
      <translation>Kut zamaha polarne distribucije.
Negativni kut stvara polarni uzorak u suprotnom smjeru.
Maksimalna apsolutna vrijednost je 360 stupnjeva.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="58"/>
      <source>Polar angle</source>
      <translation>Polarni kut</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="86"/>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="97"/>
      <source>Number of elements in the array, including a copy of the original object.
It must be at least 2.</source>
      <translation>Broj elemenata u matrici, uključujući kopiju izvornog objekta.
Mora biti najmanje 2.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="90"/>
      <source>Number of elements</source>
      <translation>Broj elemenata</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="113"/>
      <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
      <translation>Koordinate točke kroz koju prolazi os rotacije.
Promijenite smjer same osi u uređivaču svojstava.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="117"/>
      <source>Center of rotation</source>
      <translation>Centar rotacije</translation>
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
      <translation>Ponovno postavite koordinate središta rotacije.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="190"/>
      <source>Reset point</source>
      <translation>Obnovi točku</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="202"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Ako je označeno, rezultirajući objekti u matrici bit će spojeni ako se dodiruju.
Ovo djeluje samo ako je "Povezana matrica" isključen.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="206"/>
      <source>Fuse</source>
      <translation>Spoji</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="213"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Ako je označeno, rezultirajući objekt bit će "Povezana matrica" umjesto uobičajenog polja.
Povezana matrica efikasnija je pri stvaranju više kopija, ali ne može ih se spojiti zajedno.
</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="217"/>
      <source>Link array</source>
      <translation>Povezana matrica</translation>
    </message>
  </context>
  <context>
    <name>DraftShapeStringGui</name>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="26"/>
      <source>ShapeString</source>
      <translation>Oblik Teksta</translation>
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
      <translation>Unesite koordinate ili odaberite točku sa mišem.</translation>
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
      <translation>Obnovi odabir 3D točke</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="120"/>
      <source>Reset Point</source>
      <translation>Obnovi točku</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="131"/>
      <source>String</source>
      <translation>Tekst (string)</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="138"/>
      <source>Text to be made into ShapeString</source>
      <translation>Tekst koji će biti prebačen u ShapeString</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="149"/>
      <source>Height</source>
      <translation>Visina</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="156"/>
      <source>Height of the result</source>
      <translation>Visina rezultata</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="176"/>
      <source>Font file</source>
      <translation>Datoteka pisma</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddConstruction</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="309"/>
      <source>Add to Construction group</source>
      <translation>Dodavanje grupe konstrukcije</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="312"/>
      <source>Adds the selected objects to the construction group,
and changes their appearance to the construction style.
It creates a construction group if it doesn't exist.</source>
      <translation>Dodaje odabrane objekte u konstrukcijsku grupu,
te mijenja njihov izgled prema stilu gradnje.
Stvara konstrukcijsku grupu ako ne postoji.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddNamedGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="361"/>
      <source>Add a new named group</source>
      <translation>Dodajte novu imenovanu grupu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="365"/>
      <source>Add a new group with a given name.</source>
      <translation>Dodajte novu grupu s određenim imenom.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddPoint</name>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="57"/>
      <source>Add point</source>
      <translation>Dodaj točku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="60"/>
      <source>Adds a point to an existing Wire or B-spline.</source>
      <translation>Dodaje točku na postojeću žicu ili B-krivulju.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddToGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="73"/>
      <source>Move to group...</source>
      <translation type="unfinished">Move to group...</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="76"/>
      <source>Moves the selected objects to an existing group, or removes them from any group.
Create a group first to use this tool.</source>
      <translation>Premješta odabrane objekte u postojeću grupu ili ih uklanja iz bilo koje grupe.
Prvo stvorite grupu da biste koristili ovaj alat.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AnnotationStyleEditor</name>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="78"/>
      <source>Annotation styles...</source>
      <translation>Stilovi napomena...</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="82"/>
      <source>Manage or create annotation styles</source>
      <translation>Upravljajte ili stvarajte stilove bilješki</translation>
    </message>
  </context>
  <context>
    <name>Draft_ApplyStyle</name>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="47"/>
      <source>Apply current style</source>
      <translation>Primijeni aktualni stil</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="50"/>
      <source>Applies the current style defined in the toolbar (line width and colors) to the selected objects and groups.</source>
      <translation>Primjenjuje trenutni stil definiran na alatnoj traci (širina linije i boje) na odabrane objekte i grupe.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Arc</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="66"/>
      <source>Arc</source>
      <translation>Luk</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="69"/>
      <source>Creates a circular arc by a center point and a radius.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Stvara kružni luk kod točke središta i polumjera. CTRL za privući, SHIFT za ograničiti.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ArcTools</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="606"/>
      <source>Arc tools</source>
      <translation>Alati luka</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="609"/>
      <source>Create various types of circular arcs.</source>
      <translation>Napravite razne vrste kružnih lukova.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Arc_3Points</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="487"/>
      <source>Arc by 3 points</source>
      <translation>Luk iz 3 točke</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="490"/>
      <source>Creates a circular arc by picking 3 points.
CTRL to snap, SHIFT to constrain.</source>
      <translation type="unfinished">Creates a circular arc by picking 3 points.
CTRL to snap, SHIFT to constrain.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Array</name>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="68"/>
      <source>Array</source>
      <translation>Polje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="71"/>
      <source>Creates an array from a selected object.
By default, it is a 2x2 orthogonal array.
Once the array is created its type can be changed
to polar or circular, and its properties can be modified.</source>
      <translation>Stvara matricu od odabranog objekta.
Prema zadanim postavkama, to je ortogonalna matrica 2x2.
Nakon što je matrica kreirana, njen se tip može promijeniti
na polarni ili kružni, a njena svojstva se mogu mijenjati.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ArrayTools</name>
    <message>
      <location filename="../../draftguitools/gui_arrays.py" line="65"/>
      <source>Array tools</source>
      <translation>Alat matrice</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arrays.py" line="68"/>
      <source>Create various types of arrays, including rectangular, polar, circular, path, and point</source>
      <translation>Stvara različite vrste nizova, uključujući pravokutne, polarne, kružne, putanje i točke</translation>
    </message>
  </context>
  <context>
    <name>Draft_AutoGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="208"/>
      <source>Autogroup</source>
      <translation>Automatska grupa</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="211"/>
      <source>Select a group to add all Draft and Arch objects to.</source>
      <translation>Odaberite grupu u koju želite dodati sve objekte Nacrt i Luk.</translation>
    </message>
  </context>
  <context>
    <name>Draft_BSpline</name>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="60"/>
      <source>B-spline</source>
      <translation>B-krivulja</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="63"/>
      <source>Creates a multiple-point B-spline. CTRL to snap, SHIFT to constrain.</source>
      <translation>Stvara više-točaka B-krivulju. CTRL za privući, SHIFT za ograničiti.</translation>
    </message>
  </context>
  <context>
    <name>Draft_BezCurve</name>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="64"/>
      <source>Bézier curve</source>
      <translation type="unfinished">Bézier curve</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="67"/>
      <source>Creates an N-degree Bézier curve. The more points you pick, the higher the degree.
CTRL to snap, SHIFT to constrain.</source>
      <translation type="unfinished">Creates an N-degree Bézier curve. The more points you pick, the higher the degree.
CTRL to snap, SHIFT to constrain.</translation>
    </message>
  </context>
  <context>
    <name>Draft_BezierTools</name>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="475"/>
      <source>Bézier tools</source>
      <translation type="unfinished">Bézier tools</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="478"/>
      <source>Create various types of Bézier curves.</source>
      <translation type="unfinished">Create various types of Bézier curves.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Circle</name>
    <message>
      <location filename="../../draftguitools/gui_circles.py" line="80"/>
      <source>Circle</source>
      <translation>Krug</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_circles.py" line="84"/>
      <source>Creates a circle (full circular arc).
CTRL to snap, ALT to select tangent objects.</source>
      <translation>Stvara krug (puni kružni luk).
CTRL za hvatanje, ALT za odabir tangentnih objekata.</translation>
    </message>
  </context>
  <context>
    <name>Draft_CircularArray</name>
    <message>
      <location filename="../../draftguitools/gui_circulararray.py" line="65"/>
      <source>Circular array</source>
      <translation>Kružna matrica</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_circulararray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in a radial pattern
creating various circular layers.

The array can be turned into an orthogonal or a polar array by changing its type.</source>
      <translation>Stvara kopije odabranog objekta i postavlja kopije u radijalni uzorak
stvaranje raznih kružnih slojeva.

Niz se može pretvoriti u ortogonalni ili polarni niz promjenom njegovog tipa.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Clone</name>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="70"/>
      <source>Clone</source>
      <translation>Klon</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="73"/>
      <source>Creates a clone of the selected objects.
The resulting clone can be scaled in each of its three directions.</source>
      <translation>Stvara klon odabranih objekata.
Rezultirajući klon može se skalirati u svakom od svoja tri smjera.</translation>
    </message>
  </context>
  <context>
    <name>Draft_CubicBezCurve</name>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="242"/>
      <source>Cubic Bézier curve</source>
      <translation type="unfinished">Cubic Bézier curve</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="245"/>
      <source>Creates a Bézier curve made of 2nd degree (quadratic) and 3rd degree (cubic) segments. Click and drag to define each segment.
After the curve is created you can go back to edit each control point and set the properties of each knot.
CTRL to snap, SHIFT to constrain.</source>
      <translation type="unfinished">Creates a Bézier curve made of 2nd degree (quadratic) and 3rd degree (cubic) segments. Click and drag to define each segment.
After the curve is created you can go back to edit each control point and set the properties of each knot.
CTRL to snap, SHIFT to constrain.</translation>
    </message>
  </context>
  <context>
    <name>Draft_DelPoint</name>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="89"/>
      <source>Remove point</source>
      <translation>Ukloni točku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="92"/>
      <source>Removes a point from an existing Wire or B-spline.</source>
      <translation>Uklanja točku iz postojeće žice ili B-krivulje.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Dimension</name>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="84"/>
      <source>Dimension</source>
      <translation>Dimenzija</translation>
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
      <translation>Stvara dimenziju.

- Odaberite tri točke kako biste stvorili jednostavnu linearnu dimenziju.
- Odaberite ravnu liniju da biste stvorili linearnu dimenziju povezanu s tom crtom.
- Odaberite luk ili krug da biste stvorili polumjer ili dimenziju promjera povezanu s tim lukom.
- Odaberite dvije ravne linije kako biste između njih stvorili kutnu dimenziju.
CTRL za privuci, SHIFT za ograničiti, ALT za odabir ruba ili luka.

Prije pokretanja ove naredbe možete odabrati jednu liniju ili jedan kružni luk
za stvaranje odgovarajuće povezane dimenzije.
Također možete odabrati objekt 'App::IzmjeriUdaljenost' prije pokretanja ove naredbe
da ga pretvorite u objekt 'Dimenzije Nacrta'.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Downgrade</name>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="58"/>
      <source>Downgrade</source>
      <translation>Vraćanje na niži stupanj</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="61"/>
      <source>Downgrades the selected objects into simpler shapes.
The result of the operation depends on the types of objects, which may be able to be downgraded several times in a row.
For example, it explodes the selected polylines into simpler faces, wires, and then edges. It can also subtract faces.</source>
      <translation>Vraća odabrane objekte u jednostavnije oblike.
Rezultat operacije ovisi o vrstama objekata, koji se mogu vratiti unazad nekoliko puta za redom.
Na primjer, razdjeljivanje odabrane izlomljene linije u jednostavnija lica, žice, a zatim rubove. Također može oduzimati lica.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Draft2Sketch</name>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="57"/>
      <source>Draft to Sketch</source>
      <translation>Nacrt u Skicu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="60"/>
      <source>Convert bidirectionally between Draft objects and Sketches.
Many Draft objects will be converted into a single non-constrained Sketch.
However, a single sketch with disconnected traces will be converted into several individual Draft objects.</source>
      <translation>Pretvorite dvosmjerno između objekata nacrta i skica.
Mnogi objekti nacrta bit će pretvoreni u jednu neograničenu skicu.
Međutim, jedna skica s nepovezanim tragovima bit će pretvorena u nekoliko pojedinačnih objekata nacrta.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Drawing</name>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="71"/>
      <source>Drawing</source>
      <translation>Crtež</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="74"/>
      <source>Creates a 2D projection on a Drawing Workbench page from the selected objects.
This command is OBSOLETE since the Drawing Workbench became obsolete in 0.17.
Use TechDraw Workbench instead for generating technical drawings.</source>
      <translation>Stvara 2D projekciju na stranici Radni stol za crtanje od odabranih objekata.
Ova naredba je ZASTARJELA budući da je Radni stol za crtanje zastario u 0.17.
Umjesto toga upotrijebite Tehničko Crtanje radnu površinu za generiranje tehničkih crteža.
</translation>
    </message>
  </context>
  <context>
    <name>Draft_Edit</name>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="275"/>
      <source>Edit</source>
      <translation>Uredi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="284"/>
      <source>Edits the active object.
Press E or ALT+LeftClick to display context menu
on supported nodes and on supported objects.</source>
      <translation>Uređuje aktivni objekt.
Pritisnite E ili ALT+lijevi klik za prikaz kontekstnog izbornika
na podržanim čvorovima i na podržanim objektima.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Ellipse</name>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="59"/>
      <source>Ellipse</source>
      <translation>Elipsa</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="62"/>
      <source>Creates an ellipse. CTRL to snap.</source>
      <translation>Napravi elipsu. CTRL za privuci.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Facebinder</name>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="60"/>
      <source>Facebinder</source>
      <translation>Poveznice lica</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="62"/>
      <source>Creates a facebinder object from selected faces.</source>
      <translation>Stvara objekt poveznica lica iz odabranih lica.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Fillet</name>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="64"/>
      <source>Fillet</source>
      <translation>Obrubi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="67"/>
      <source>Creates a fillet between two selected wires or edges.</source>
      <translation>Stvara obrub između dvije odabrane žice ili rubova.</translation>
    </message>
  </context>
  <context>
    <name>Draft_FlipDimension</name>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="62"/>
      <source>Flip dimension</source>
      <translation>Preokreni dimenziju</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="66"/>
      <source>Flip the normal direction of the selected dimensions (linear, radial, angular).
If other objects are selected they are ignored.</source>
      <translation>Preokrenite normalni smjer odabranih dimenzija (linearne, radijalne, kutne).
Ako su odabrani drugi objekti, oni se zanemaruju.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Hatch</name>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="38"/>
      <source>Hatch</source>
      <translation>Šrafura</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="42"/>
      <source>Creates hatches on the faces of a selected object</source>
      <translation>Stvara šrafure na licima odabranog objekta</translation>
    </message>
  </context>
  <context>
    <name>Draft_Heal</name>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="58"/>
      <source>Heal</source>
      <translation>Izliječiti</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="61"/>
      <source>Heal faulty Draft objects saved with an earlier version of the program.
If an object is selected it will try to heal that object in particular,
otherwise it will try to heal all objects in the active document.</source>
      <translation>Izliječite neispravne objekte skice spremljene ranijom verzijom programa.
Ako je objekt odabran, pokušat će izliječiti posebno taj objekt,
inače će pokušati izliječiti sve objekte u aktivnom dokumentu.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Join</name>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="66"/>
      <source>Join</source>
      <translation>Spoji</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="69"/>
      <source>Joins the selected lines or polylines into a single object.
The lines must share a common point at the start or at the end for the operation to succeed.</source>
      <translation>Spaja odabrane linije ili izlomljene linije u jedan objekt.
Linije moraju dijeliti zajedničku točku na početku ili na kraju da bi operacija uspjela.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Label</name>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="64"/>
      <source>Label</source>
      <translation>Oznaka</translation>
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
      <translation>Stvara oznaku, opcionalno priloženu odabranom objektu ili pod-elementu.

Prvo odaberite vrh, rub ili lice objekta, a zatim pozovite ovu naredbu,
a zatim postavite položaj vodeće linije i tekstualne oznake.
Oznaka će moći prikazati informacije o ovom objektu, te o odabranom pod-elementu,
ako postoje.

Ako je odabrano previše objekata ili previše pod-elemenata, samo prvi u svakom slučaju
će se koristit za pružanje informacija na oznaci.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Layer</name>
    <message>
      <location filename="../../draftguitools/gui_layers.py" line="52"/>
      <source>Layer</source>
      <translation>Sloj</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_layers.py" line="55"/>
      <source>Adds a layer to the document.
Objects added to this layer can share the same visual properties such as line color, line width, and shape color.</source>
      <translation>Dodaje sloj u dokument.
Objekti dodani ovom sloju mogu dijeliti ista vizualna svojstva kao što su boja linije, širina linije i boja oblika.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Line</name>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="63"/>
      <source>Line</source>
      <translation>Linija</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="66"/>
      <source>Creates a 2-point line. CTRL to snap, SHIFT to constrain.</source>
      <translation>Napravi liniju pomoću dvije točke. CTRL za privuci, SHIFT za ograničiti.</translation>
    </message>
  </context>
  <context>
    <name>Draft_LinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="119"/>
      <source>LinkArray</source>
      <translation>PovezanaMatrica</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="122"/>
      <source>Like the Array tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>Poput alata Matrica, ali umjesto toga stvara 'Poveznice Matrica'.
'Poveznice Matrica' je učinkovitije pri rukovanju sa puno kopija, ali opcija 'Spoji' se ne može koristiti.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Mirror</name>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="63"/>
      <source>Mirror</source>
      <translation>Zrcaljenje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="66"/>
      <source>Mirrors the selected objects along a line defined by two points.</source>
      <translation>Zrcaljenje odabranih objekata duž linije definirane kroz dvije točke.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Move</name>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="63"/>
      <source>Move</source>
      <translation>Pomicanje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="66"/>
      <source>Moves the selected objects from one base point to another point.
If the "copy" option is active, it will create displaced copies.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Pomiče odabrane objekte s jedne osnovne točke na drugu točku.
Ako je opcija "copy" aktivna, stvorit će pomaknute kopije.
CTRL za privuci, SHIFT za ograničiti.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Offset</name>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="63"/>
      <source>Offset</source>
      <translation>Pomak</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="66"/>
      <source>Offsets of the selected object.
It can also create an offset copy of the original object.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
      <translation>Pomaci odabranog objekta.
Također može stvoriti pomak kopiju izvornog objekta.
CTRL za privuci, SHIFT za ograničiti. Držite ALT i kliknite za stvaranje kopije svakim klikom.

</translation>
    </message>
  </context>
  <context>
    <name>Draft_OrthoArray</name>
    <message>
      <location filename="../../draftguitools/gui_orthoarray.py" line="65"/>
      <source>Array</source>
      <translation>Polje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_orthoarray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in an orthogonal pattern,
meaning the copies follow the specified direction in the X, Y, Z axes.

The array can be turned into a polar or a circular array by changing its type.</source>
      <translation>Stvara kopije odabranog objekta i postavlja kopije u ortogonalni uzorak,
što znači da kopije slijede navedeni smjer u X, Y, Z osi.

Niz se može pretvoriti u polarni ili kružni niz promjenom njegovog tipa.

</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathArray</name>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="73"/>
      <source>Path array</source>
      <translation>Matrica Puta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="76"/>
      <source>Creates copies of the selected object along a selected path.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
      <translation>Stvara kopije odabranog objekta duž odabrane staze.
Najprije odaberite objekt, a zatim odaberite stazu.
Put može biti izlomljena linija, B-krivulja, Bezierova krivulja ili čak rubovi drugih objekata.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="163"/>
      <source>Path Link array</source>
      <translation>Poveznica matrice Puta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="166"/>
      <source>Like the PathArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>Poput alata MatricaPuta, ali umjesto toga stvara 'Poveznice Matrica'.
'Poveznice Matrica' je učinkovitije pri rukovanju sa puno kopija, ali opcija 'Spoji' se ne može koristiti.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathTwistedArray</name>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="66"/>
      <source>Path twisted array</source>
      <translation>Matrica zaokrenuta po putu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="69"/>
      <source>Creates copies of the selected object along a selected path, and twists the copies.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
      <translation>Stvara kopije odabranog objekta duž odabrane staze i zaokreće kopije.
Najprije odaberite objekt, a zatim odaberite stazu.
Put može biti izlomljena linija, B-krivulja, Bezierova krivulja ili čak rubovi drugih objekata.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathTwistedLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="121"/>
      <source>Path twisted Link array</source>
      <translation>Povezana Matrica zaokrenuta po putu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="124"/>
      <source>Like the PathTwistedArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>Poput alata Povezana Matrica Puta, ali umjesto toga stvara 'Poveznice Matrica'.
'Poveznice Matrica' je učinkovitije pri rukovanju sa puno kopija, ali opcija 'Spoji' se ne može koristiti.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Point</name>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="62"/>
      <source>Point</source>
      <translation>Točka</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="65"/>
      <source>Creates a point object. Click anywhere on the 3D view.</source>
      <translation>Stvara točkasti objekt. Kliknite bilo gdje na 3D prikazu.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PointArray</name>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="76"/>
      <source>Point array</source>
      <translation>Matrica točaka</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="79"/>
      <source>Creates copies of the selected object, and places the copies at the position of various points.

The points need to be grouped under a compound of points before using this tool.
To create this compound, select various points and then use the Part Compound tool,
or use the Draft Upgrade tool to create a 'Block', or create a Sketch and add simple points to it.

Select the base object, and then select the compound or the sketch to create the point array.</source>
      <translation>Stvara kopije odabranog objekta i postavlja kopije na položaj različitih točaka.

Točke je potrebno grupirati pod spoj točaka prije korištenja ovog alata.
Da biste stvorili ovaj spoj, odaberite različite točke, a zatim upotrijebite alat Složen spoj,
ili upotrijebite alat za Nadogradnju nacrta da biste stvorili 'Blok', ili izradite Skicu i dodajte joj jednostavne Točke.

Odaberite Osnovni objekt, a zatim odaberite Spoj ili Skicu da biste stvorili Matricu točaka.

</translation>
    </message>
  </context>
  <context>
    <name>Draft_PointLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="141"/>
      <source>PointLinkArray</source>
      <translation>MatricaPovezanihTočaka</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="144"/>
      <source>Like the PointArray tool, but creates a 'Point link array' instead.
A 'Point link array' is more efficient when handling many copies.</source>
      <translation>Poput alata MatricaTočaka, ali umjesto toga stvara 'Matrica povezanih točaka'.
'Matrica povezanih točaka' je učinkovitiji pri rukovanju sa puno  kopija.

</translation>
    </message>
  </context>
  <context>
    <name>Draft_PolarArray</name>
    <message>
      <location filename="../../draftguitools/gui_polararray.py" line="65"/>
      <source>Polar array</source>
      <translation>Polarna matrica</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polararray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in a polar pattern
defined by a center of rotation and its angle.

The array can be turned into an orthogonal or a circular array by changing its type.</source>
      <translation>Stvara kopije odabranog objekta i postavlja kopije u polarni uzorak
definirane središtem rotacije i njegovim kutom.

Matrica se može pretvoriti u ortogonalnu ili kružnu matricu sa promjenom njenog tipa.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Polygon</name>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="57"/>
      <source>Polygon</source>
      <translation>Poligon</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="60"/>
      <source>Creates a regular polygon (triangle, square, pentagon, ...), by defining the number of sides and the circumscribed radius.
CTRL to snap, SHIFT to constrain</source>
      <translation>Stvara pravilan poligon (trokut, kvadrat, peterokut, ...), definiranjem broja stranica i opisanog polumjera.
CTRL za privuci, SHIFT za ograničiti

</translation>
    </message>
  </context>
  <context>
    <name>Draft_Rectangle</name>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="54"/>
      <source>Rectangle</source>
      <translation>Pravokutnik</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="57"/>
      <source>Creates a 2-point rectangle. CTRL to snap.</source>
      <translation>Napravi  s 2 točke pravokutnik. CTRL za privuci.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Rotate</name>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="62"/>
      <source>Rotate</source>
      <translation>Rotiraj</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="65"/>
      <source>Rotates the selected objects. Choose the center of rotation, then the initial angle, and then the final angle.
If the "copy" option is active, it will create rotated copies.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
      <translation>Rotira odabrane objekte. Odaberite središte rotacije, zatim početni kut, a zatim konačni kut.
Ako je opcija "copy" aktivna, stvorit će se rotirane kopije.
CTRL za privuci, SHIFT za ograničiti. Držite ALT i kliknite za stvaranje kopije svakim klikom.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Scale</name>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="71"/>
      <source>Scale</source>
      <translation>Skaliraj</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="74"/>
      <source>Scales the selected objects from a base point.
CTRL to snap, SHIFT to constrain, ALT to copy.</source>
      <translation>Skalira odabrane objekte od osnovne točke.
CTRL za privuci, SHIFT za ograničiti, ALT za kopiranje.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SelectGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="164"/>
      <source>Select group</source>
      <translation>Odabir grupe</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="167"/>
      <source>Selects the contents of selected groups. For selected non-group objects, the contents of the group they are in is selected.</source>
      <translation>Odabire sadržaj odabranih grupa. Za odabrane objekte koji nisu u grupi, odabire se sadržaj grupe u kojoj se nalaze.</translation>
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
      <translation>Odaberite lice čvrstog tijela kako biste stvorili radnu ravninu na kojoj ćete skicirati skice objekata.
Također možete odabrati tri vrha ili proxy radne ravnine.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SetStyle</name>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="55"/>
      <source>Set style</source>
      <translation>Postavi stil</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="57"/>
      <source>Sets default styles</source>
      <translation>Postavlja zadani stil</translation>
    </message>
  </context>
  <context>
    <name>Draft_Shape2DView</name>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="59"/>
      <source>Shape 2D view</source>
      <translation>Pogled 2D oblika</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="62"/>
      <source>Creates a 2D projection of the selected objects on the XY plane.
The initial projection direction is the negative of the current active view direction.
You can select individual faces to project, or the entire solid, and also include hidden lines.
These projections can be used to create technical drawings with the TechDraw Workbench.</source>
      <translation>Stvara 2D projekciju odabranih objekata na XY ravnini.
Početni smjer projekcije je negativan na trenutni aktivni smjer pogleda.
Možete odabrati pojedinačna lica za projiciranje ili cijelo tijelo, i također uključiti skrivene linije.
Ove se projekcije mogu koristiti za izradu tehničkih crteža s TehničkoCrtanje radnim stolom.

</translation>
    </message>
  </context>
  <context>
    <name>Draft_ShapeString</name>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="66"/>
      <source>Shape from text</source>
      <translation>Oblik iz teksta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="68"/>
      <source>Creates a shape from a text string by choosing a specific font and a placement.
The closed shapes can be used for extrusions and boolean operations.</source>
      <translation>Stvara oblik iz tekstualnog niza odabirom određenog fonta i položaja.
Zatvoreni oblici mogu se koristiti za ekstrudiranje i booleove operacije.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ShowSnapBar</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="589"/>
      <source>Show snap toolbar</source>
      <translation>Prikaži alatnu traku Hvatanje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="592"/>
      <source>Show the snap toolbar if it is hidden.</source>
      <translation>Prikazuje alatnu traku Hvatanje ako je skrivena.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Slope</name>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="70"/>
      <source>Set slope</source>
      <translation>Postavite nagib</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="73"/>
      <source>Sets the slope of the selected line by changing the value of the Z value of one of its points.
If a polyline is selected, it will apply the slope transformation to each of its segments.

The slope will always change the Z value, therefore this command only works well for
straight Draft lines that are drawn in the XY plane. Selected objects that aren't single lines will be ignored.</source>
      <translation>Postavlja nagib odabrane linije mijenjajući vrijednost Z vrijednosti jedne od njezinih točaka.
Ako je izlomljena linija odabrana, ona će primijeniti transformaciju nagiba na svaki od svojih segmenata.

Nagib će uvijek promijeniti vrijednost Z, stoga ova naredba dobro funkcionira samo za
ravne linije Nacrta koje su nacrtane u ravnini XY. Odabrani objekti koji nisu pojedinačne linije bit će zanemareni.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Angle</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="344"/>
      <source>Angle</source>
      <translation>Kut</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="347"/>
      <source>Set snapping to points in a circular arc located at multiples of 30 and 45 degree angles.</source>
      <translation>Postavite hvatanje na točke u kružnom luku smještene pod kutovima od 30 i 45 stupnjeva.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Center</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="374"/>
      <source>Center</source>
      <translation>Središte</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="377"/>
      <source>Set snapping to the center of a circular arc.</source>
      <translation>Postavite Hvatanje na središte kružnog luka.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Dimensions</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="526"/>
      <source>Show dimensions</source>
      <translation>Prikaži dimenzije</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="529"/>
      <source>Show temporary linear dimensions when editing an object and using other snapping methods.</source>
      <translation>Prikažite privremene linearne dimenzije kada uređujete objekt i koristite druge metode hvatanja.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Endpoint</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="313"/>
      <source>Endpoint</source>
      <translation>Krajnja točka</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="316"/>
      <source>Set snapping to endpoints of an edge.</source>
      <translation>Postavite Hvatanje na krajnje točke ruba.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Extension</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="404"/>
      <source>Extension</source>
      <translation>Proširenje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="407"/>
      <source>Set snapping to the extension of an edge.</source>
      <translation>Postavite Hvatanje na produžetak ruba.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Grid</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="223"/>
      <source>Grid</source>
      <translation>Rešetka</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="226"/>
      <source>Set snapping to the intersection of grid lines.</source>
      <translation>Postavite Hvatanje na sjecište linija mreže.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Intersection</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="253"/>
      <source>Intersection</source>
      <translation>Presjek</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="256"/>
      <source>Set snapping to the intersection of edges.</source>
      <translation>Postavite Hvatanje na raskrižje rubova.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Lock</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="133"/>
      <source>Main snapping toggle On/Off</source>
      <translation>Glavni prekidač za Hvatanje uključi/isključi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="136"/>
      <source>Activates or deactivates all snap methods at once.</source>
      <translation>Aktivira ili deaktivira sve metode Hvatanja odjednom.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Midpoint</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="163"/>
      <source>Midpoint</source>
      <translation>Srednja točka</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="166"/>
      <source>Set snapping to the midpoint of an edge.</source>
      <translation>Postavite Hvatanje na srednju točka ruba.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Near</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="434"/>
      <source>Nearest</source>
      <translation>Najbliži</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="437"/>
      <source>Set snapping to the nearest point of an edge.</source>
      <translation>Postavite Hvatanje na najbližu točku ruba.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Ortho</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="465"/>
      <source>Orthogonal</source>
      <translation>Ortogonalno</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="468"/>
      <source>Set snapping to a direction that is a multiple of 45 degrees from a point.</source>
      <translation>Postavite Hvatanje na smjer koji je višekratnik od 45 stupnjeva od točke.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Parallel</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="283"/>
      <source>Parallel</source>
      <translation>Paralelno</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="286"/>
      <source>Set snapping to a direction that is parallel to an edge.</source>
      <translation>Postavite Hvatanje u smjeru koji je paralelan s rubom.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Perpendicular</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="193"/>
      <source>Perpendicular</source>
      <translation>Okomito</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="196"/>
      <source>Set snapping to a direction that is perpendicular to an edge.</source>
      <translation>Postavite Hvatanje u smjeru koji je okomito s rubom.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Special</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="495"/>
      <source>Special</source>
      <translation>Posebno</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="498"/>
      <source>Set snapping to the special points defined inside an object.</source>
      <translation>Postavite Hvatanje na posebne točke definirane unutar objekta.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_WorkingPlane</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="559"/>
      <source>Working plane</source>
      <translation>Radna ravnina</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="562"/>
      <source>Restricts snapping to a point in the current working plane.
If you select a point outside the working plane, for example, by using other snapping methods,
it will snap to that point's projection in the current working plane.</source>
      <translation>Ograničava Hvatanje na točku u trenutnoj radnoj ravnini.
Ako odaberete točku izvan radne ravnine, na primjer, korištenjem drugih metoda hvatanja,
skočit će na projekciju te točke u trenutnoj radnoj ravnini.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Split</name>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="56"/>
      <source>Split</source>
      <translation>Razdjeli</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="59"/>
      <source>Splits the selected line or polyline into two independent lines
or polylines by clicking anywhere along the original object.
It works best when choosing a point on a straight segment and not a corner vertex.</source>
      <translation>Odabranu liniju ili izlomljenu liniju dijeli na dvije neovisne linije
ili izlomljene linije klikom bilo gdje duž izvornog objekta.
Najbolje funkcionira kada odaberete točku na ravnom segmentu, a ne na kutnom vrhu.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Stretch</name>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="64"/>
      <source>Stretch</source>
      <translation>Razvuci</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="67"/>
      <source>Stretches the selected objects.
Select an object, then draw a rectangle to pick the vertices that will be stretched,
then draw a line to specify the distance and direction of stretching.</source>
      <translation>Razvlači odabrane objekte.
Odaberite objekt, a zatim nacrtajte pravokutnik da odaberete vrhove koji će biti rastegnuti,
zatim nacrtajte liniju kako biste odredili udaljenost i smjer istezanja.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SubelementHighlight</name>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="61"/>
      <source>Subelement highlight</source>
      <translation>Istakni Pod-element</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="64"/>
      <source>Highlight the subelements of the selected objects, so that they can then be edited with the move, rotate, and scale tools.</source>
      <translation>Istakni pod-elemente odabranih objekata kako bi se mogli uređivati ​​pomoću alata za pomicanje, rotiranje i skaliranje.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Text</name>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="60"/>
      <source>Text</source>
      <translation>Tekst</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="63"/>
      <source>Creates a multi-line annotation. CTRL to snap.</source>
      <translation>Stvara više-linijsku napomenu. CTRL za privuci.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleConstructionMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="99"/>
      <source>Toggle construction mode</source>
      <translation>Uključivanje/isključivanje konstrukcijskog moda</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="102"/>
      <source>Toggles the Construction mode.
When this is active, the following objects created will be included in the construction group, and will be drawn with the specified color and properties.</source>
      <translation>Prebacuje Način izgradnje.
Kada je ovo aktivno, sljedeći stvoreni objekti bit će uključeni u skupinu konstrukcija i bit će nacrtani navedenom bojom i svojstvima.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleContinueMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="131"/>
      <source>Toggle continue mode</source>
      <translation>Uključivanje/Isključivanje načina Nastaviti</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="133"/>
      <source>Toggles the Continue mode.
When this is active, any drawing tool that is terminated will automatically start again.
This can be used to draw several objects one after the other in succession.</source>
      <translation>Prebacuje način rada za Nastavak.
Kada je ovo aktivno, svaki alat za crtanje koji je prekinut automatski će se ponovno pokrenuti.
Ovo se može koristiti za crtanje nekoliko objekata jedan za drugim uzastopno.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleDisplayMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="166"/>
      <source>Toggle normal/wireframe display</source>
      <translation>Uključite normalni/žičani prikaz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="168"/>
      <source>Switches the display mode of selected objects from flatlines to wireframe and back.
This is helpful to quickly visualize objects that are hidden by other objects.
This is intended to be used with closed shapes and solids, and doesn't affect open wires.</source>
      <translation>Prebacuje način prikaza odabranih objekata s ravnih linija na žičani okvir i natrag.
Ovo je korisno za brzo vizualiziranje objekata koji su skriveni od drugih objekata.
Ovo je namijenjeno za korištenje sa zatvorenim oblicima i čvrstim tvarima i ne utječe na otvorene žice.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleGrid</name>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="58"/>
      <source>Toggle grid</source>
      <translation>Pokaži/Sakrij rešetku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="59"/>
      <source>Toggles the Draft grid on and off.</source>
      <translation>Uključuje/isključuje rešetku nacrta (on/off)</translation>
    </message>
  </context>
  <context>
    <name>Draft_Trimex</name>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="76"/>
      <source>Trimex</source>
      <translation>Produži</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="82"/>
      <source>Trims or extends the selected object, or extrudes single faces.
CTRL snaps, SHIFT constrains to current segment or to normal, ALT inverts.</source>
      <translation>Obrezuje ili proširuje odabrani objekt ili istiskuje pojedinačna lica.
CTRL uhvati, SHIFT ograničiti trenutni segment kao normalan, ALT preokreće.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Upgrade</name>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="58"/>
      <source>Upgrade</source>
      <translation>Nadogradi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="61"/>
      <source>Upgrades the selected objects into more complex shapes.
The result of the operation depends on the types of objects, which may be able to be upgraded several times in a row.
For example, it can join the selected objects into one, convert simple edges into parametric polylines,
convert closed edges into filled faces and parametric polygons, and merge faces into a single face.</source>
      <translation>Nadograđuje odabrane objekte u složenije oblike.
Rezultat operacije ovisi o vrsti objekata, koji se mogu nadograditi nekoliko puta za redom.
Na primjer, može spojiti odabrane objekte u jedan, pretvoriti jednostavne rubove u parametarske izlomljene linije,
pretvoriti zatvorene rubove u ispunjena lica i parametarske poligone i spojiti lica u jedno lice.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Wire</name>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="306"/>
      <source>Polyline</source>
      <translation>Linija segmenata</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="309"/>
      <source>Creates a multiple-points line (polyline). CTRL to snap, SHIFT to constrain.</source>
      <translation>Stvara više-točaka krivulju (izlomljena linija). CTRL za hvatanje, SHIFT za ograničiti.</translation>
    </message>
  </context>
  <context>
    <name>Draft_WireToBSpline</name>
    <message>
      <location filename="../../draftguitools/gui_wire2spline.py" line="64"/>
      <source>Wire to B-spline</source>
      <translation>Žica u B-krivulju</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_wire2spline.py" line="67"/>
      <source>Converts a selected polyline to a B-spline, or a B-spline to a polyline.</source>
      <translation>Pretvara odabranu izlomljenu liniju u B-krivulju ili B-krivulju u izlomljenu liniju.</translation>
    </message>
  </context>
  <context>
    <name>Draft_WorkingPlaneProxy</name>
    <message>
      <location filename="../../draftguitools/gui_planeproxy.py" line="50"/>
      <source>Create working plane proxy</source>
      <translation>Napravi radnu ravninu proxy </translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_planeproxy.py" line="52"/>
      <source>Creates a proxy object from the current working plane.
Once the object is created double click it in the tree view to restore the camera position and objects' visibilities.
Then you can use it to save a different camera position and objects' states any time you need.</source>
      <translation>Stvara proxy objekt iz trenutne radne ravnine.
Nakon što je objekt stvoren, dvaput ga kliknite u prikazu stabla kako biste vratili položaj kamere i vidljivost objekata.
Zatim ga možete koristiti za spremanje drugog položaja kamere i stanja objekata kad god vam zatreba.</translation>
    </message>
  </context>
  <context>
    <name>Form</name>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="14"/>
      <source>Working plane setup</source>
      <translation>Postavljanje Radne ravnine</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="20"/>
      <source>Select a face or working plane proxy or 3 vertices.
Or choose one of the options below</source>
      <translation>Odaberite lice,  zamjensku radnu ravninu ili 3 vrha.
Ili odaberite jednu od donjih opcija</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="31"/>
      <source>Sets the working plane to the XY plane (ground plane)</source>
      <translation>Postavlja radnu ravninu na XY ravninu (osnovna ravnina)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="34"/>
      <source>Top (XY)</source>
      <translation>Vrh (XY)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="41"/>
      <source>Sets the working plane to the XZ plane (front plane)</source>
      <translation>Postavlja radnu ravninu na XZ ravninu (prednja ravnina)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="44"/>
      <source>Front (XZ)</source>
      <translation>Lice (XZ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="51"/>
      <source>Sets the working plane to the YZ plane (side plane)</source>
      <translation>Postavlja radnu ravninu na YZ ravninu (bočna ravnina)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="54"/>
      <source>Side (YZ)</source>
      <translation>Strana (YZ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="61"/>
      <source>Sets the working plane facing the current view</source>
      <translation>Postavlja radnu ravninu prema trenutnom pogledu</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="64"/>
      <source>Align to view</source>
      <translation>Poravnaj sa pogledom</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="71"/>
      <source>The working plane will align to the current
view each time a command is started</source>
      <translation>Radna ravnina će se uskladiti sa trenutnim
pogledom svaki put kad se naredba pokrene</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="78"/>
      <source>Automatic</source>
      <translation>Automatsko</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="87"/>
      <source>Offset</source>
      <translation>Pomak</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="94"/>
      <source>An optional offset to give to the working plane
above its base position. Use this together with one
of the buttons above</source>
      <translation>Jedan neobvezni pomak za dodavanje radnoj ravnini
iznad svog osnovnog položaja. Upotrijebite ovo zajedno s jednim
od gornjih gumba</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="106"/>
      <location filename="../ui/TaskSelectPlane.ui" line="118"/>
      <source>If this is selected, the working plane will be
centered on the current view when pressing one
of the buttons above</source>
      <translation>Ako je odabrano, radna ravnina će biti
centrirana na trenutni pogled kada pritisnete jedan
od gornjih gumba</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="111"/>
      <source>Center plane on view</source>
      <translation>Centriraj ravninu na pogled</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="135"/>
      <source>Or select a single vertex to move the current
working plane without changing its orientation.
Then, press the button below</source>
      <translation>Ili odaberite jedan vrh za pomicanje trenutne
radne ravnine bez promjene orijentacije.
Zatim pritisnite gumb ispod
</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="147"/>
      <source>Moves the working plane without changing its
orientation. If no point is selected, the plane
will be moved to the center of the view</source>
      <translation>Pomiče radnu ravninu bez promjene njezine
orijentacija. Ako nije odabrana nijedna točka, ravnina
će biti premještena u središte pogleda
</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="152"/>
      <source>Move working plane</source>
      <translation>Pomakni radnu ravninu</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="161"/>
      <location filename="../ui/TaskSelectPlane.ui" line="171"/>
      <source>The spacing between the smaller grid lines</source>
      <translation>Razmak između manjih linija rešetke</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="164"/>
      <source>Grid spacing</source>
      <translation>Veličina rešetke</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="181"/>
      <location filename="../ui/TaskSelectPlane.ui" line="191"/>
      <source>The number of squares between each main line of the grid</source>
      <translation>Broj kvadrata između svake glavne crte rešetke</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="184"/>
      <source>Main line every</source>
      <translation>Glavna linija svakih</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="198"/>
      <source>Grid extension</source>
      <translation>Proširenje mreže

</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="205"/>
      <source> lines</source>
      <translation> linije</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="218"/>
      <location filename="../ui/TaskSelectPlane.ui" line="230"/>
      <source>The distance at which a point can be snapped to
when approaching the mouse. You can also change this
value by using the [ and ] keys while drawing</source>
      <translation>Udaljenost u kojoj se može privući na točku
kad joj se miš približi. To također možete promijeniti
tijekom crtanja pomoću tipki [i]
</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="223"/>
      <source>Snapping radius</source>
      <translation>Privuci-na radijus</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="241"/>
      <source>Centers the view on the current working plane</source>
      <translation>Centrira pogled na trenutnu radnu ravninu</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="244"/>
      <source>Center view</source>
      <translation>Centriraj pogled</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="251"/>
      <source>Resets the working plane to its previous position</source>
      <translation>Vraća radnu ravninu na prijašnji položaj</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="254"/>
      <source>Previous</source>
      <translation>Prethodno</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="14"/>
      <source>Style settings</source>
      <translation>Postavke stila</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="22"/>
      <source>Fills the values below with a stored style preset</source>
      <translation>Ispunjava vrijednosti u nastavku pohranjenim unaprijed postavljenim stilom</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="26"/>
      <source>Load preset</source>
      <translation>Učitaj predefinirane postavke</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="40"/>
      <source>Save current style as a preset...</source>
      <translation>Spremi trenutni stil kao unaprijed postavljenu...</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="57"/>
      <source>Lines and faces</source>
      <translation>Linije i naličja</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="66"/>
      <source>Line color</source>
      <translation>Boja linije</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="73"/>
      <source>The color of lines</source>
      <translation>Boja linija</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="80"/>
      <source>Line width</source>
      <translation>Širina linije</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="87"/>
      <source> px</source>
      <translation> px</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="94"/>
      <source>Draw style</source>
      <translation>Stil crtanja</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="101"/>
      <source>The line style</source>
      <translation>Stil linije</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="105"/>
      <source>Solid</source>
      <translation>Čvrsto tijelo</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="110"/>
      <source>Dashed</source>
      <translation>Iscrtkano</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="115"/>
      <source>Dotted</source>
      <translation>Točkasto</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="120"/>
      <source>DashDot</source>
      <translation>Crtica Točka</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="128"/>
      <source>Display mode</source>
      <translation>Način prikaza</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="135"/>
      <source>The display mode for faces</source>
      <translation>Način prikaza za lica</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="139"/>
      <source>Flat Lines</source>
      <translation>Ravne linije</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="144"/>
      <source>Wireframe</source>
      <translation>Žičana mreža</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="149"/>
      <source>Shaded</source>
      <translation>Osijenčeno</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="154"/>
      <source>Points</source>
      <translation>Točke</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="162"/>
      <source>Shape color</source>
      <translation>Boja oblika</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="169"/>
      <source>The color of faces</source>
      <translation>Boja lica</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="176"/>
      <source>Transparency</source>
      <translation>Prozirnost</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="183"/>
      <source>The transparency of faces</source>
      <translation>Prozirnost lica

</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="186"/>
      <source> %</source>
      <translation> %</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="199"/>
      <source>Annotations</source>
      <translation>Napomene</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="205"/>
      <source>Text font</source>
      <translation>Pismo (font) teksta</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="218"/>
      <source>The font to use for texts and dimensions</source>
      <translation>Pismo (font) koji se koristi za tekst i dimenzije</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="225"/>
      <source>Text size</source>
      <translation>Veličina teksta</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="232"/>
      <source>The size of texts and dimension texts</source>
      <translation>Veličina teksta i teksta dimenzija</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="242"/>
      <source>Text spacing</source>
      <translation>Razmak teksta</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="249"/>
      <source>The space between the text and the dimension line</source>
      <translation>Razmak između teksta i redka dimenzije</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="259"/>
      <source>Text color</source>
      <translation>Boja teksta</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="266"/>
      <source>The color of texts and dimension texts</source>
      <translation>Boja teksta i teksta dimenzija</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="273"/>
      <source>Line spacing</source>
      <translation>Razmak između redova</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="280"/>
      <source>The spacing between different lines of text</source>
      <translation>Razmak između redaka teksta</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="287"/>
      <source>Arrow style</source>
      <translation>Stil strelice</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="294"/>
      <source>The type of dimension arrows</source>
      <translation>Tip strelica dimenzija

</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="298"/>
      <source>Dot</source>
      <translation>Točka</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="303"/>
      <source>Circle</source>
      <translation>Krug</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="308"/>
      <source>Arrow</source>
      <translation>Strelica</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="313"/>
      <source>Tick</source>
      <translation>Okomita crtica</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="318"/>
      <source>Tick-2</source>
      <translation>Okomita crtica-2</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="326"/>
      <source>Arrow size</source>
      <translation>Veličina strelice</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="333"/>
      <source>The size of dimension arrows</source>
      <translation>Veličina strelica dimenzija

</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="343"/>
      <source>Show unit</source>
      <translation>Prikaži jedinice mjere</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="350"/>
      <source>If the unit suffix is shown on dimension texts or not</source>
      <translation>Dali je jedinični sufiks prikazan na dimenzijskim tekstovima ili ne

</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="363"/>
      <source>Unit override</source>
      <translation>Mijenja jedinicu mjere</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="370"/>
      <source>The unit to use for dimensions. Leave blank to use current FreeCAD unit</source>
      <translation>Jedinica za dimenzije. Ostavite prazno da biste koristili trenutnu FreeCAD jedinicu

</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="382"/>
      <source>Apply above style to selected object(s)</source>
      <translation>Primijeni gornji stil na odabrane objekt(e)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="385"/>
      <source>Selected</source>
      <translation>Odabrano</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="397"/>
      <source>Texts/dims</source>
      <translation>Tekstovi/dims</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="14"/>
      <source>Form</source>
      <translation>Obrazac</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="20"/>
      <source>PAT file:</source>
      <translation>PAT datoteka:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="27"/>
      <source>pattern files (*.pat)</source>
      <translation>datoteke uzoraka (pattern) (*.pat)</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="34"/>
      <source>Pattern:</source>
      <translation>Uzorak:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="44"/>
      <source>Scale</source>
      <translation>Skaliraj</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="64"/>
      <source>Rotation:</source>
      <translation>Rotacija:</translation>
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
      <translation>Glavne postavke</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="32"/>
      <source>General Draft Settings</source>
      <translation>Glavne postavke nacrta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="40"/>
      <source>Default working plane</source>
      <translation>Zadana ravnina rada</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="67"/>
      <source>None</source>
      <translation>Prazno</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="72"/>
      <source>XY (Top)</source>
      <translation>XY (Gore)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="77"/>
      <source>XZ (Front)</source>
      <translation>XZ (Naprijed)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="82"/>
      <source>YZ (Side)</source>
      <translation>YZ (Strana)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="94"/>
      <source>Internal precision level</source>
      <translation>Interni nivo preciznosti</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="120"/>
      <source>The number of decimals in internal coordinates operations (for ex. 3 = 0.001). Values between 6 and 8 are usually considered the best trade-off among FreeCAD users.</source>
      <translation>Broj decimala u operacijama unutarnjih koordinata (na primjer 3 = 0,001). Vrijednosti između 6 i 8 obično se smatraju najboljim kompromisom među FreeCAD korisnicima.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="143"/>
      <source>Tolerance</source>
      <translation>Tolerancija</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="169"/>
      <source>This is the value used by functions that use a tolerance.
Values with differences below this value will be treated as same. This value will be obsoleted soon so the precision level above controls both.</source>
      <translation>Ovo je vrijednost koju koriste funkcije koje koriste toleranciju.
Vrijednosti s razlikama ispod ove vrijednosti tretirat će se kao iste. Ova će vrijednost uskoro zastarjeti pa će razina preciznosti biti iznad oba.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="194"/>
      <source>If this option is checked, the layers drop-down list will also show groups, allowing you to automatically add objects to groups too.</source>
      <translation>Ako je ova opcija potvrđena, dijaloški okvir također će prikazati grupe koje vam omogućuju i da automatski dodate objekte u grupe.
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="197"/>
      <source>Show groups in layers list drop-down button</source>
      <translation>Prikaži grupe u dijaloškom okviru popisa slojeva</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="216"/>
      <source>Draft tools options</source>
      <translation>Opcije alata Nacrta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="230"/>
      <source>When drawing lines, set focus on Length instead of X coordinate.
This allows to point the direction and type the distance.</source>
      <translation>Prilikom crtanja crta, postavite fokus na dužinu umjesto na X koordinatu.
Ovo omogućava prikazivanje smjera i unos udaljenosti.
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="234"/>
      <source>Set focus on Length instead of X coordinate</source>
      <translation>Postavite fokus na dužinu umjesto na X koordinatu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="247"/>
      <source>Normally, after copying objects, the copies get selected.
If this option is checked, the base objects will be selected instead.</source>
      <translation>Nakon kopiranja objekata obično su odabrane kopije.
Ako je ova opcija označena, umjesto njih će se odabrati osnovni objekti.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="251"/>
      <source>Select base objects after copying</source>
      <translation>Odaberite bazu objekata nakon kopiranja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="264"/>
      <source>If this option is set, when creating Draft objects on top of an existing face of another object, the "Support" property of the Draft object will be set to the base object. This was the standard behaviour before FreeCAD 0.19</source>
      <translation>Ako je ova opcija postavljena, pri kreiranju nacrta objekata na postojećem licu drugog objekta, svojstvo "Podrška" nacrta objekta bit će postavljeno na osnovni objekt. Ovo je bilo standardno ponašanje prije FreeCAD 0.19</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="267"/>
      <source>Set the Support property when possible</source>
      <translation>Postavite značajku podrške kada je to moguće</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="280"/>
      <source>If this is checked, objects will appear as filled by default.
Otherwise, they will appear as wireframe</source>
      <translation>Ako je to označeno, objekti će se pojaviti ispunjeni prema zadanim postavkama. Inače, oni će se pojaviti kao žičani model</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="284"/>
      <source>Fill objects with faces whenever possible</source>
      <translation>Ispunite objekte s licima kad god je moguće</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="300"/>
      <source>If this is checked, copy mode will be kept across command,
otherwise commands will always start in no-copy mode</source>
      <translation>Ako je to potvrđeno, način kopiranja zadržat će se preko naredbe,
inače se naredbe uvijek pokreću u načinu bez kopiranja
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="304"/>
      <source>Global copy mode</source>
      <translation>Globalni način kopiranja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="320"/>
      <source>Force Draft Tools to create Part primitives instead of Draft objects.
Note that this is not fully supported, and many object will be not editable with Draft Modifiers.</source>
      <translation>Prisilite Alate za Nacrt da biste stvorili primitivne Dijelove umjesto objekata nacrta.
Imajte na umu da to nije u potpunosti podržano i mnogi objekti neće se moći uređivati pomoću Modifikatora Nacrta.
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="324"/>
      <source>Use Part Primitives when available</source>
      <translation>Koristite primitivni dio kada su dostupni</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="341"/>
      <source>Prefix labels of Clones with:</source>
      <translation>Prefiks oznake klonova sa:</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="372"/>
      <source>Construction Geometry</source>
      <translation>Konstrukcijska Geometrija</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="380"/>
      <source>Construction group name</source>
      <translation>Ime grupe konstrukcije</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="387"/>
      <source>This is the default group name for construction geometry</source>
      <translation>Ovo je zadani naziv grupe za izgradnju geometrije</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="390"/>
      <source>Construction</source>
      <translation>Izgradnja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="407"/>
      <source>Construction geometry color</source>
      <translation>Boja konstrukcijske geometrije</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="427"/>
      <source>This is the default color for objects being drawn while in construction mode.</source>
      <translation>Ovo je zadana boja za objekte koji se izrađuju u modu izgradnje.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="14"/>
      <source>Visual settings</source>
      <translation>Vizualne postavke</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="26"/>
      <source>Visual Settings</source>
      <translation>Vizualne Postavke</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="34"/>
      <source>Snap symbols style</source>
      <translation>Stil simbola privuci</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="54"/>
      <source>Draft classic style</source>
      <translation>Nacrt klasični stil</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="59"/>
      <source>Bitsnpieces style</source>
      <translation>Bitsnpieces stil</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="80"/>
      <source>Color</source>
      <translation>Boja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="87"/>
      <source>The default color for snap symbols</source>
      <translation>Zadana boja za privuci simbole</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="111"/>
      <source>Check this if you want to use the color/linewidth from the toolbar as default</source>
      <translation>Odaberite ovo ako želite koristiti boje/širina linije na alatnoj traci kao standardno</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="114"/>
      <source>Save current color and linewidth across sessions</source>
      <translation>Spremi trenutnu boju i širinu linije na sve objekte u ovoj sesiji</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="131"/>
      <source>If checked, a widget indicating the current working plane orientation appears during drawing operations</source>
      <translation>Ako je označeno, dodatak pokazuje orijentaciju trenutne radne površine, pojavljuje se tijekom operacija crtanja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="134"/>
      <source>Show Working Plane tracker</source>
      <translation>Prikaži tragač radne površine</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="151"/>
      <source>Default template sheet</source>
      <translation>Zadani predložak lista</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="177"/>
      <source>The default template to use when creating a new drawing sheet</source>
      <translation>Zadani predložak pri stvaranju novog crteža</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="194"/>
      <source>Alternate SVG patterns location</source>
      <translation>Položaj alternativnih SVG uzoraka</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="220"/>
      <source>Here you can specify a directory with custom SVG files containing &lt;pattern&gt; definitions to be added to the standard patterns</source>
      <translation>Ovdje možete odrediti direktorij s prilagođenim SVG datotekama koje sadrže definicije &lt;pattern&gt; koje će se dodati standardnim uzorcima</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="237"/>
      <source>SVG pattern resolution</source>
      <translation>Rezolucija SVG uzorka</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="257"/>
      <source>The resolution to draw the patterns in. Default value is 128. Higher values give better resolutions, lower values make drawing faster</source>
      <translation>Razlučivost za crtanje uzoraka. Zadana vrijednost je 128. Više vrijednosti daju bolje rezolucije, a niže vrijednosti čine brže crtanje

</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="280"/>
      <source>SVG pattern default size</source>
      <translation>Zadana veličina SVG uzorka</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="300"/>
      <source>The default size for SVG patterns</source>
      <translation>Zadana veličina za SVG uzorak</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="326"/>
      <source>Check this if you want to preserve colors of faces while doing downgrade and upgrade (splitFaces and makeShell only)</source>
      <translation>Označite za očuvanje boje naličja za vrijeme vraćanja na staru verziju i nadogradnje (razdjeli lica i napravi samo ljuske)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="329"/>
      <source>Preserve colors of faces during downgrade/upgrade</source>
      <translation>Sačuvaj boje naličja za vrijeme vraćanja na staru verziju/nadogradnje</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="346"/>
      <source>Check this if you want the face names to derive from the originating object name and vice versa while doing downgrade/upgrade (splitFaces and makeShell only)</source>
      <translation>Označite to ako želite da imena lica potječu iz izvornog imena objekta i obrnuto dok radite vraćanje na stariju verziju / nadogradnju (samo splitFaces i makeShell)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="349"/>
      <source>Preserve names of faces during downgrade/upgrade</source>
      <translation>Sačuvaj imena naličja za vrijeme vraćanja na staru verziju/nadogradnje</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="367"/>
      <source>Drawing view line definitions</source>
      <translation>Definicije vidljivih linija crteža</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="375"/>
      <source>Dashed line definition</source>
      <translation>Iscrtkana linija definicija</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="395"/>
      <location filename="../ui/preferences-draftvisual.ui" line="438"/>
      <location filename="../ui/preferences-draftvisual.ui" line="481"/>
      <source>An SVG linestyle definition</source>
      <translation>SVG definicija stila linije</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="398"/>
      <source>0.09,0.05</source>
      <translation>0.09,0.05</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="418"/>
      <source>Dashdot line definition</source>
      <translation>Crtica-točka definicija linije</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="441"/>
      <source>0.09,0.05,0.02,0.05</source>
      <translation>0.09,0.05,0.02,0.05</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="461"/>
      <source>Dotted line definition</source>
      <translation>Točkasta linija definicija</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="484"/>
      <source>0.02,0.02</source>
      <translation>0.02,0.02</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="14"/>
      <source>Texts and dimensions</source>
      <translation>Tekstovi i dimenzije</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="26"/>
      <source>Text settings</source>
      <translation>Postavke teksta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="34"/>
      <source>Font family</source>
      <translation>Obitelj Fontova</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="47"/>
      <source>This is the default font name for all Draft texts and dimensions.
It can be a font name such as "Arial", a default style such as "sans", "serif"
or "mono", or a family such as "Arial,Helvetica,sans" or a name with a style
such as "Arial:Bold"</source>
      <translation>Ovo je zadani font za sve tekstove nacrta i dimenzije.
To može biti font kao što je "Arial", stil kao što su "sans", "serif"
ili "mono", ili skupina kao što su "Arial, Helvetica, sans" ili ime sa stilom,
kao što su "Arial:Bold"</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="59"/>
      <source>Internal font</source>
      <translation>Interno pismo</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="76"/>
      <source>Font size</source>
      <translation>Veličina Pisma</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="89"/>
      <source>Default height for texts and dimensions</source>
      <translation>Zadana visina teksta i dimenzije</translation>
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
      <translation>Postavke dimenzija</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="124"/>
      <source>Display mode</source>
      <translation>Način prikaza</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="144"/>
      <source>text above (2D)</source>
      <translation>tekst iznad (2D)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="149"/>
      <source> text inside (3D)</source>
      <translation> tekst unutar (3D)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="161"/>
      <source>Number of decimals</source>
      <translation>Broj decimala</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="201"/>
      <source>Extension lines size</source>
      <translation>Veličina linije protezanja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="208"/>
      <source>The default size of dimensions extension lines</source>
      <translation>Zadana veličina dimenzije produljenih linija</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="237"/>
      <source>Extension line overshoot</source>
      <translation>Produžetak linije protezanja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="244"/>
      <source>The default length of extension line above dimension line</source>
      <translation>Zadana dužina linije protezanja iznad linije dimenzije</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="273"/>
      <source>Dimension line overshoot</source>
      <translation>Linija dimenzije prekoračenje</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="280"/>
      <source>The default distance the dimension line is extended past extension lines</source>
      <translation>Zadana udaljenost linije dimenzije je proširena preko linije protezanja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="309"/>
      <source>Arrows style</source>
      <translation>Vrsta strelice</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="323"/>
      <source>Dot</source>
      <translation>Točka</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="328"/>
      <source>Circle</source>
      <translation>Krug</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="333"/>
      <source>Arrow</source>
      <translation>Strelica</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="338"/>
      <source>Tick</source>
      <translation>Okomita crtica</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="343"/>
      <source>Tick-2</source>
      <translation>Okomita crtica-2</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="355"/>
      <source>Arrows size</source>
      <translation>Veličina strelice</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="362"/>
      <source>The default size of arrows</source>
      <translation>Zadane veličine strelice</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="388"/>
      <source>Text orientation</source>
      <translation>Smjer teksta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="395"/>
      <source>This is the orientation of the dimension texts when those dimensions are vertical. Default is left, which is the ISO standard.</source>
      <translation>Ovo je orijentacija tekstova na dimenzijama kada su one okomite. Zadano je lijevo, što je ISO standard.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="405"/>
      <source>Left (ISO standard)</source>
      <translation>Lijevo (ISO standard)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="410"/>
      <source>Right</source>
      <translation>Desno</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="422"/>
      <source>Text spacing</source>
      <translation>Razmak teksta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="429"/>
      <source>The space between the dimension line and the dimension text</source>
      <translation>Razmak između linije dimenzije i teksta dimenzije</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="455"/>
      <source>Show the unit suffix in dimensions</source>
      <translation>Prikaži jedinice dodatka u dimenzijama</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="475"/>
      <source>Override unit</source>
      <translation>Poništi mjernu jedinicu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="495"/>
      <source>By leaving this field blank, the dimension measurements will be shown in the current unit defined in FreeCAD. By indicating a unit here such as m or cm, you can force new dimensions to be shown in that unit.</source>
      <translation>Ostavljanjem ovog polja praznim, mjerenja dimenzija bit će prikazana u trenutnoj jedinici definiranoj u FreeCAD-u. Označavanjem jedinice ovdje kao što je m ili cm, možete prisiliti prikazivanje novih dimenzija u toj mjernoj jedinici.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="513"/>
      <source>ShapeString settings</source>
      <translation>Postavke tekstualnog oblika</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="521"/>
      <source>Default ShapeString font file</source>
      <translation>Zadani font datoteke tekstualnog oblika</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="534"/>
      <source>Select a font file</source>
      <translation>Odaberite datoteku pisma</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="14"/>
      <source>SVG</source>
      <translation>SVG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="40"/>
      <source>Import style</source>
      <translation>Uvezi stil</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="47"/>
      <source>Method chosen for importing SVG object color to FreeCAD</source>
      <translation>Metoda odabrana je za uvoz SVG boje objekta u FreeCAD-u</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="60"/>
      <source>None (fastest)</source>
      <translation>Nijedan (najbrži)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="65"/>
      <source>Use default color and linewidth</source>
      <translation>Koristite zadanu boju i širinu linije</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="70"/>
      <source>Original color and linewidth</source>
      <translation>Izvorna boja i širina linije</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="82"/>
      <source>If checked, no units conversion will occur.
One unit in the SVG file will translate as one millimeter. </source>
      <translation>Ako ovo uključeno, neće doći do pretvorbe jedinica.
Jedna jedinica u SVG datoteci će se prevesti kao jedan milimetar. </translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="86"/>
      <source>Disable units scaling</source>
      <translation>Onemogući skaliranje jedinica</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="121"/>
      <source>Export style</source>
      <translation>Izvezi stil</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="128"/>
      <source>Style of SVG file to write when exporting a sketch</source>
      <translation>Stil  za pisanje SVG datoteke kod izvoza skice</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="141"/>
      <source>Translated (for print &amp; display)</source>
      <translation>Pomaknuto (za ispis i pregled)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="146"/>
      <source>Raw (for CAM)</source>
      <translation>Raw (za CAM)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="158"/>
      <source>All white lines will appear in black in the SVG for better readability against white backgrounds</source>
      <translation>Sve će bijele linije u SVG-u biti crne boje radi bolje čitljivosti na bijelim pozadinama</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="161"/>
      <source>Translate white line color to black</source>
      <translation>Prevedi bijelu boju linije u crnu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="181"/>
      <source>Max segment length for discretized arcs</source>
      <translation>Maks. dužina segmenta za odvojene lukove</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="204"/>
      <source>Versions of Open CASCADE older than version 6.8 don't support arc projection.
In this case arcs will be discretized into small line segments.
This value is the maximum segment length. </source>
      <translation>Verzije Open CASCADE starije od verzije 6.8 ne podržavaju projekciju luka.
U ovom će slučaju lukovi biti odvojeni u male segmente linija.
Ova vrijednost je maksimalna duljina segmenta.
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="14"/>
      <source>OCA</source>
      <translation>OCA</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="46"/>
      <source>Check this if you want the areas (3D faces) to be imported too.</source>
      <translation>Označite ovo ako želite područja (3D površine) da se također učitavaju.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="49"/>
      <source>Import OCA areas</source>
      <translation>Uvoz OCA područja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="14"/>
      <source>DXF</source>
      <translation>DXF</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="35"/>
      <source>This preferences dialog will be shown when importing/ exporting DXF files</source>
      <translation>Ovaj dijalog postavki prikazat će se prilikom uvoza / izvoza DXF datoteka
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="38"/>
      <source>Show this dialog when importing and exporting</source>
      <translation>Prikaži ovaj dijalog kod uvoza i kod izvoza</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="51"/>
      <source>Python importer is used, otherwise the newer C++ is used.
Note: C++ importer is faster, but is not as featureful yet</source>
      <translation>Koristi se Python uvoznik, a inače se koristi noviji C ++.
Napomena: C ++ uvoznik  je brži, ali još nije tako dobro dorađen</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="55"/>
      <source>Use legacy python importer</source>
      <translation>Korištenje naslijeđenog python uvoznika</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="71"/>
      <source>Python exporter is used, otherwise the newer C++ is used.
Note: C++ exporter is faster, but is not as featureful yet</source>
      <translation>Koristi se Python izvoznik, a inače se koristi noviji C ++.
Napomena: C ++ izvoznik je brži, ali još nije tako dobro dorađen</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="75"/>
      <source>Use legacy python exporter</source>
      <translation>Korištenje naslijeđenog python izvoznika</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="88"/>
      <source>Automatic update (legacy importer only)</source>
      <translation>Automatska nadogradnja (samo naslijeđeni uvoznik)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="96"/>
      <source>Allow FreeCAD to download the Python converter for DXF import and export.
You can also do this manually by installing the "dxf_library" workbench
from the Addon Manager.</source>
      <translation>Omogućite FreeCAD-u da preuzme Python konverter za DXF uvoz i izvoz.
To možete učiniti i ručno instaliranjem radne površine "dxf_library"
iz Upravitelja dodataka</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="101"/>
      <source>Allow FreeCAD to automatically download and update the DXF libraries</source>
      <translation>Omogući FreeCADu da automatski učita i nadogradi DXF biblioteke</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="26"/>
      <location filename="../ui/preferences-dxf.ui" line="119"/>
      <location filename="../ui/preferences-svg.ui" line="26"/>
      <source>Import options</source>
      <translation>Postavke uvoza</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="140"/>
      <source>Note: Not all the options below are used by the new importer yet</source>
      <translation>Napomena: Još nisu sve opcije ispod korištene kod novog uvoznika</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="149"/>
      <source>Import</source>
      <translation>Uvoz</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="156"/>
      <source>If unchecked, texts and mtexts won't be imported</source>
      <translation>Ako je ovo isključeno, neće se uvesti tekstovi i mtekstovi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="159"/>
      <source>texts and dimensions</source>
      <translation>tekstovi i dimenzije</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="172"/>
      <source>If unchecked, points won't be imported</source>
      <translation>Ako nije označeno, točke se neće uvesti</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="175"/>
      <source>points</source>
      <translation>točke</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="188"/>
      <source>If checked, paper space objects will be imported too</source>
      <translation>Ako je označeno, papir objekta prostora se isto uvozi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="191"/>
      <source>layouts</source>
      <translation>izgledi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="204"/>
      <source>If you want the non-named blocks (beginning with a *) to be imported too</source>
      <translation>Ako želite da se uvoze i neimenovani blokovi (koji počinju s *)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="207"/>
      <source>*blocks</source>
      <translation>* blokovi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="224"/>
      <source>Create</source>
      <translation>Stvoriti</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="231"/>
      <source>Only standard Part objects will be created (fastest)</source>
      <translation>Stvorit će se samo standardni objekti (najbrže)
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="234"/>
      <source>simple Part shapes</source>
      <translation>jednostavan dio oblika</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="250"/>
      <source>Parametric Draft objects will be created whenever possible</source>
      <translation>Parametarski objekti Nacrta  kreirat će se kad god je to moguće
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="253"/>
      <source>Draft objects</source>
      <translation>Skice objekata</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="266"/>
      <source>Sketches will be created whenever possible</source>
      <translation>Skice će se napraviti kad god je to moguće</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="269"/>
      <source>Sketches</source>
      <translation>Skice</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="289"/>
      <source>Scale factor to apply to imported files</source>
      <translation>Faktor skaliranja za dodavanje u uvežene datoteke</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="309"/>
      <source>Scale factor to apply to DXF files on import.
The factor is the conversion between the unit of your DXF file and millimeters.
Example: for files in millimeters: 1, in centimeters: 10,
                             in meters: 1000, in inches: 25.4, in feet: 304.8</source>
      <translation>Faktor razmjere koji se primjenjuje na DXF datoteke pri uvozu.
Faktor je konverzija između jedinice DXF datoteke i milimetra.
Primjer: za datoteke u milimetrima: 1, u centimetrima: 10,
                              u metrima: 1000, u inčima: 25,4, u stopalima: 304,8
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="338"/>
      <source>Colors will be retrieved from the DXF objects whenever possible.
Otherwise default colors will be applied. </source>
      <translation>Boje će  biti dohvaćene iz DXF objekata kad god je to moguće.
Inače će se primijeniti zadane boje. </translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="342"/>
      <source>Get original colors from the DXF file</source>
      <translation>Uzima orginalne boje iz DXF datoteke</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="359"/>
      <source>FreeCAD will try to join coincident objects into wires.
Note that this can take a while!</source>
      <translation>FreeCAD će pokušati pridružiti podudarne objekte u žice.
Pazi, ovo može potrajati!</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="363"/>
      <source>Join geometry</source>
      <translation>Pridružite geometriju</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="380"/>
      <source>Objects from the same layers will be joined into Draft Blocks,
turning the display faster, but making them less easily editable </source>
      <translation>Objekti istih slojeva će biti spojeni u blokove nacrta,
prikazuju se brže ali su kompliciraniji za uređivanje </translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="384"/>
      <source>Group layers into blocks</source>
      <translation>Grupiranje slojeva u blokove</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="401"/>
      <source>Imported texts will get the standard Draft Text size,
instead of the size they have in the DXF document</source>
      <translation>Uvezeni tekstovi dobit će standardnu veličinu nacrta teksta,
umjesto veličine koju imaju u DXF dokumentu
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="405"/>
      <source>Use standard font size for texts</source>
      <translation>Koristi standardnu veličinu fonta za tekst</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="422"/>
      <source>If this is checked, DXF layers will be imported as Draft Layers</source>
      <translation>Ako je to označeno, DXF slojevi se uvoze kao slojevi Nacrta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="425"/>
      <source>Use Layers</source>
      <translation>Koristi Slojeve</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="445"/>
      <source>Hatches will be converted into simple wires</source>
      <translation>Šrafiranje će se pretvoriti u jednostavne žice</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="448"/>
      <source>Import hatch boundaries as wires</source>
      <translation>očitaj krajeve otvora kao žice</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="465"/>
      <source>If polylines have a width defined, they will be rendered
as closed wires with correct width</source>
      <translation>Ako su poli-linije definirane širinom, one će se prikazati
kao zatvorene žice sa ispravnom širinom
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="469"/>
      <source>Render polylines with width</source>
      <translation>Renderiraj polilinije sa debljinom</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="486"/>
      <source>Ellipse export is poorly supported. Use this to export them as polylines instead.</source>
      <translation>Izvoz elipse slabo je podržan. Upotrijebite ovo da biste ih izvezli kao linije segmenata (polilinije).</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="489"/>
      <source>Treat ellipses and splines as polylines</source>
      <translation>Obrađuje elipse i krivulje (spline) kao polyline (zlomljenu liniju)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="518"/>
      <source>Max Spline Segment:</source>
      <translation>Max dužina segmenta (spline) krivulje:</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="528"/>
      <source>Maximum length of each of the polyline segments.
If it is set to '0' the whole spline is treated as a straight segment.</source>
      <translation>Maksimalna duljina svakog od segmenata polylinije.
Ako je postavljeno na '0', cijela je krivulja tretirana kao ravni segment.
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="559"/>
      <location filename="../ui/preferences-svg.ui" line="107"/>
      <source>Export options</source>
      <translation>Postavke izvoza</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="567"/>
      <source>All objects containing faces will be exported as 3D polyfaces</source>
      <translation>Svi objekti koji sadrže lica izvozit će se u obliku 3D lica
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="570"/>
      <source>Export 3D objects as polyface meshes</source>
      <translation>Izvoz 3D objekata kao polyface mreže</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="587"/>
      <source>Drawing Views will be exported as blocks.
This might fail for post DXF R12 templates.</source>
      <translation>Pogledi crteža izvozit će se kao blokovi.
Ovo možda neće uspjeti za predloške DXF R12.
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="591"/>
      <source>Export Drawing Views as blocks</source>
      <translation>Izvoz pogleda Crteža kao blokovi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="611"/>
      <source>Exported objects will be projected to reflect the current view direction</source>
      <translation>Izvezeni objekti projicirat će se tako da odražavaju trenutni smjer pogleda</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="614"/>
      <source>Project exported objects along current view direction</source>
      <translation>Projiciraj izvezene objekte duž trenutnog smjera pogleda</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="14"/>
      <source>Grid and snapping</source>
      <translation>Rešetka i privuci na</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="35"/>
      <source>Snapping</source>
      <translation>Privuci</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="43"/>
      <source>If this is checked, snapping is activated without the need to press the snap mod key</source>
      <translation>Ako je to označeno, privuci na se aktivira bez potrebe pritiska na tipku privuci na</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="46"/>
      <source>Always snap (disable snap mod)</source>
      <translation>Uvijek privuci (onesposobi privuci prekidač mod)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="66"/>
      <source>Constrain mod</source>
      <translation>Rad sa ograničenjem</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="86"/>
      <source>The Constraining modifier key</source>
      <translation>Tipka za ograničavanje</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="96"/>
      <location filename="../ui/preferences-draftsnap.ui" line="151"/>
      <location filename="../ui/preferences-draftsnap.ui" line="206"/>
      <source>Shift</source>
      <translation>Shift - Tipka</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="101"/>
      <location filename="../ui/preferences-draftsnap.ui" line="156"/>
      <location filename="../ui/preferences-draftsnap.ui" line="211"/>
      <source>Ctrl</source>
      <translation>Ctrl - Tipka</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="106"/>
      <location filename="../ui/preferences-draftsnap.ui" line="161"/>
      <location filename="../ui/preferences-draftsnap.ui" line="216"/>
      <source>Alt</source>
      <translation>Alt - Tipka</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="118"/>
      <source>Snap mod</source>
      <translation>Rad s rešetkom</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="138"/>
      <source>The snap modifier key</source>
      <translation>Tipka za rešetku</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="173"/>
      <source>Alt mod</source>
      <translation>Alt mod</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="193"/>
      <source>The Alt modifier key</source>
      <translation>"Alt" pomoćna tipka</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="228"/>
      <source>If checked, the Snap toolbar will be shown whenever you use snapping</source>
      <translation>Ako je uključeno, Snap alatna traka će biti prikazana kada god koristite snap</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="231"/>
      <source>Show Draft Snap toolbar</source>
      <translation>Prikaži Draft snap alat</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="251"/>
      <source>Hide Draft snap toolbar after use</source>
      <translation>Sakrij Draft Snap alat nakon korištenja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="272"/>
      <source>Grid</source>
      <translation>Rešetka</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="278"/>
      <source>If checked, a grid will appear when drawing</source>
      <translation>Ako je označeno, rešetka će se pojaviti prilikom crtanja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="281"/>
      <source>Use grid</source>
      <translation>Koristi rešetku</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="300"/>
      <source>If checked, the Draft grid will always be visible when the Draft workbench is active. Otherwise only when using a command</source>
      <translation>Ako je uključeno, rešetka će uvijek biti prikazana kada je Draft okruženje aktivno. Inače samo kada se koristi naredba</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="303"/>
      <source>Always show the grid</source>
      <translation>Uvijek pokaži rešetku</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="319"/>
      <source>If checked, an additional border is displayed around the grid, showing the main square size in the bottom left border</source>
      <translation>Ako je potvrđeno, oko rešetke se prikazuje dodatan obrub koji u donjem lijevom obrubu prikazuje veličinu glavnog kvadrata

</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="322"/>
      <source>Show grid border</source>
      <translation>Pokaži obrub rešetke</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="338"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;If checked, the outline of a human figure is displayed at the bottom left corner of the grid. This option is only effective if the BIM workbench is installed and if &amp;quot;Show grid border&amp;quot; option is enabled.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation type="unfinished">&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;If checked, the outline of a human figure is displayed at the bottom left corner of the grid. This option is only effective if the BIM workbench is installed and if &amp;quot;Show grid border&amp;quot; option is enabled.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="341"/>
      <source>Show human figure</source>
      <translation type="unfinished">Show human figure</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="357"/>
      <source>If set, the grid will have its two main axes colored in red, green or blue when they match global axes</source>
      <translation>Ako je postavljena, mreža će imati dvije glavne osi obojene crvenom, zelenom ili plavom bojom kada se podudaraju s globalnim osi

</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="360"/>
      <source>Use colored axes</source>
      <translation>Koristite osi u boji</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="381"/>
      <source>Main lines every</source>
      <translation>Glavne linije svakih</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="404"/>
      <source>Mainlines will be drawn thicker. Specify here how many squares between mainlines.</source>
      <translation>Glavne linije će biti deblje crtane. Ovdje postaviti koliko kvadrata je među njima.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="430"/>
      <source>Grid spacing</source>
      <translation>Veličina rešetke</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="453"/>
      <source>The spacing between each grid line</source>
      <translation>Razmak između linija na rešetki</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="485"/>
      <source>Grid size</source>
      <translation>Veličina rešetke</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="505"/>
      <source>The number of horizontal or vertical lines of the grid</source>
      <translation>Broj vodoravnih i okomitih linija na rešetki</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="511"/>
      <source> lines</source>
      <translation> linije</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="534"/>
      <source>Grid color and transparency</source>
      <translation>Boja rešetke i prozirnost</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="554"/>
      <source>The color of the grid</source>
      <translation>Boja rešetke</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="574"/>
      <source>The overall transparency of the grid</source>
      <translation>Ukupna transparentnost mreže</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="595"/>
      <source>Draft Edit preferences</source>
      <translation>Nacrt postavke za uređivanje</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="598"/>
      <source>Edit</source>
      <translation>Uredi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="621"/>
      <source>Maximum number of contemporary edited objects</source>
      <translation>Maksimalan broj istodobno uređenih objekata</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="644"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Sets the maximum number of objects Draft Edit&lt;/p&gt;&lt;p&gt;can process at the same time&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt; &lt;p&gt; Postavlja najveći broj objekata Skica za uređivanje &lt;/p&gt; &lt;p&gt; može istovremeno obraditi &lt;/p&gt; &lt;/body&gt; &lt;/html&gt;
</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="691"/>
      <source>Draft edit pick radius</source>
      <translation>Odabir polumjera za uređivanje  nacrta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="714"/>
      <source>Controls pick radius of edit nodes</source>
      <translation>Kontrola odabira polumjera za uređivanja čvorova</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="14"/>
      <source>DWG</source>
      <translation>DWG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="26"/>
      <source>DWG conversion</source>
      <translation>DWG pretvorbe</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="34"/>
      <source>Conversion method:</source>
      <translation type="unfinished">Conversion method:</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="41"/>
      <source>This is the method FreeCAD will use to convert DWG files to DXF. If "Automatic" is chosen, FreeCAD will try to find one of the following converters in the same order as they are shown here. If FreeCAD is unable to find any, you might need to choose a specific converter and indicate its path here under. Choose the "dwg2dxf" utility if using LibreDWG, "ODAFileConverter" if using the ODA file converter, or the "dwg2dwg" utility if using the pro version of QCAD.</source>
      <translation type="unfinished">This is the method FreeCAD will use to convert DWG files to DXF. If "Automatic" is chosen, FreeCAD will try to find one of the following converters in the same order as they are shown here. If FreeCAD is unable to find any, you might need to choose a specific converter and indicate its path here under. Choose the "dwg2dxf" utility if using LibreDWG, "ODAFileConverter" if using the ODA file converter, or the "dwg2dwg" utility if using the pro version of QCAD.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="51"/>
      <source>Automatic</source>
      <translation>Automatsko</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="56"/>
      <source>LibreDWG</source>
      <translation type="unfinished">LibreDWG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="61"/>
      <source>ODA Converter</source>
      <translation type="unfinished">ODA Converter</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="66"/>
      <source>QCAD pro</source>
      <translation type="unfinished">QCAD pro</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="78"/>
      <source>Path to file converter</source>
      <translation type="unfinished">Path to file converter</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="85"/>
      <source>The path to your DWG file converter executable</source>
      <translation type="unfinished">The path to your DWG file converter executable</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="100"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=" font-weight:600;"&gt;Note:&lt;/span&gt; DXF options apply to DWG files as well.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=" font-weight:600;"&gt;Napomena:&lt;/span&gt; DXF po mogućnosti primijeniti na DWG datoteke.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="14"/>
      <source>User interface settings</source>
      <translation>Postavke korisničkog sučelja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="26"/>
      <source>In-Command Shortcuts</source>
      <translation>In-Command prečaci</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="37"/>
      <source>Relative</source>
      <translation>Relativno</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="59"/>
      <source>R</source>
      <translation>R</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="81"/>
      <source>Continue</source>
      <translation>Nastavi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="103"/>
      <source>T</source>
      <translation>T</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="125"/>
      <source>Close</source>
      <translation>Zatvori</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="147"/>
      <source>O</source>
      <translation>O</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="169"/>
      <source>Copy</source>
      <translation>Kopiraj</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="191"/>
      <source>P</source>
      <translation>P</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="213"/>
      <source>Subelement Mode</source>
      <translation>Pod-element Mod</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="235"/>
      <source>D</source>
      <translation>D</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="257"/>
      <source>Fill</source>
      <translation>Ispuni</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="279"/>
      <source>L</source>
      <translation>L</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="301"/>
      <source>Exit</source>
      <translation>Izađi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="323"/>
      <source>A</source>
      <translation>A</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="345"/>
      <source>Select Edge</source>
      <translation>Odaberite Rub</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="367"/>
      <source>E</source>
      <translation>E</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="389"/>
      <source>Add Hold</source>
      <translation>Dodaj Hvatište</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="411"/>
      <source>Q</source>
      <translation>Q</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="433"/>
      <source>Length</source>
      <translation>Dužina</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="455"/>
      <source>H</source>
      <translation>H</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="477"/>
      <source>Wipe</source>
      <translation>Obriši</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="499"/>
      <source>W</source>
      <translation>W</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="521"/>
      <source>Set WP</source>
      <translation>Postavljanje WP</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="543"/>
      <source>U</source>
      <translation>U</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="565"/>
      <source>Cycle Snap</source>
      <translation>Ciklus Privuci</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="587"/>
      <source>`</source>
      <translation>`</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="609"/>
      <source>Global</source>
      <translation>Globalna</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="631"/>
      <source>G</source>
      <translation>G</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="653"/>
      <source>Snap</source>
      <translation>Privuci</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="675"/>
      <source>S</source>
      <translation>S</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="697"/>
      <source>Increase Radius</source>
      <translation>Povećaj Polumjer</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="719"/>
      <source>[</source>
      <translation>[</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="741"/>
      <source>Decrease Radius</source>
      <translation>Smanji Polumjer</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="763"/>
      <source>]</source>
      <translation>]</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="785"/>
      <source>Restrict X</source>
      <translation>Ograničeno X</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="807"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="829"/>
      <source>Restrict Y</source>
      <translation>Ograničeno Y</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="851"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="873"/>
      <source>Restrict Z</source>
      <translation>Ograničeno Z</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="895"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="928"/>
      <source>Enable draft statusbar customization</source>
      <translation>Omogući prilagodbu  trake stanja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="931"/>
      <source>Draft Statusbar</source>
      <translation>Nacrt, traka stanja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="951"/>
      <source>Enable snap statusbar widget</source>
      <translation>Omogući dodatak trake stanja, privuci na</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="954"/>
      <source>Draft snap widget</source>
      <translation>Dodatak Privuci nacrt </translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="970"/>
      <source>Enable draft statusbar annotation scale widget</source>
      <translation>Omogući traku stanja, Nacrt,  dodatak skale obilježavanja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="973"/>
      <source>Annotation scale widget</source>
      <translation>Dodatak Napomena skale</translation>
    </message>
  </context>
  <context>
    <name>ImportAirfoilDAT</name>
    <message>
      <location filename="../../importAirfoilDAT.py" line="193"/>
      <source>Did not find enough coordinates</source>
      <translation>Nisam pronašao dovoljno koordinata</translation>
    </message>
  </context>
  <context>
    <name>ImportSVG</name>
    <message>
      <location filename="../../importSVG.py" line="1796"/>
      <source>Unknown SVG export style, switching to Translated</source>
      <translation>Nepoznati SVG stil izvoza, prebacivanje na Prevedeno</translation>
    </message>
    <message>
      <location filename="../../importSVG.py" line="1816"/>
      <source>The export list contains no object with a valid bounding box</source>
      <translation>Popis izvoza ne sadrži objekt s važećim graničnim okvirom</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../InitGui.py" line="104"/>
      <source>Draft creation tools</source>
      <translation>Alati kreiranja Nacrta</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="107"/>
      <source>Draft annotation tools</source>
      <translation>Alati napomena Nacrta</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="110"/>
      <source>Draft modification tools</source>
      <translation>Alati izmjena Nacrta</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="113"/>
      <source>Draft utility tools</source>
      <translation>Uslužni alati nacrta</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="118"/>
      <source>&amp;Drafting</source>
      <translation>&amp;Izrada Nacrta</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="121"/>
      <source>&amp;Annotation</source>
      <translation>Anotacija</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="124"/>
      <source>&amp;Modification</source>
      <translation>&amp;Modifikacija</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="127"/>
      <source>&amp;Utilities</source>
      <translation>&amp;Uslužni programi</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="50"/>
      <source>Arc tools</source>
      <translation>Alati luka</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="58"/>
      <source>Bézier tools</source>
      <translation type="unfinished">Bézier tools</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="89"/>
      <source>Array tools</source>
      <translation>Alat matrice</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snapper.py" line="1477"/>
      <source>Draft Snap</source>
      <translation>Privuci na Nacrt</translation>
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
      <translation>DXF uvoz/izvoz biblioteke koje trebaju FreeCAD
DXF formatu nisu pronađene na ovom sustavu.
Omogućite FreeCAD za preuzimanje ovih biblioteka:
  1 - Učitaj "Nacrt" Radnu Površinu
  2 - Izbornik  Uređivanja&gt; Postavke &gt; Uvoz-Izvoz &gt; DXF &gt; Omogućavanje preuzimanja
Ili ručno instalirati biblioteku dxf-Library, kao što je objašnjeno na 
https://github.com/yorikvanhavre/Draft-dxf-importer
Da bi odobrio FreeCAD-u  preuzimanje ove biblioteke, odgovori sa Da (Yes).</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="57"/>
      <location filename="../../DraftGui.py" line="751"/>
      <source>Relative</source>
      <translation>Relativno</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="61"/>
      <location filename="../../DraftGui.py" line="756"/>
      <source>Global</source>
      <translation>Globalna</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="66"/>
      <location filename="../../DraftGui.py" line="774"/>
      <location filename="../../DraftGui.py" line="1126"/>
      <source>Continue</source>
      <translation>Nastavi</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="71"/>
      <location filename="../../DraftGui.py" line="790"/>
      <source>Close</source>
      <translation>Zatvori</translation>
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
      <translation>Kopiraj</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="81"/>
      <source>Subelement mode</source>
      <translation>Pod-element mod</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="86"/>
      <source>Fill</source>
      <translation>Ispuni</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="91"/>
      <source>Exit</source>
      <translation>Izađi</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="96"/>
      <source>Snap On/Off</source>
      <translation>Privuci On/Off</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="101"/>
      <source>Increase snap radius</source>
      <translation>Smanji domet privlačenja</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="106"/>
      <source>Decrease snap radius</source>
      <translation>Smanji domet privlačenja</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="111"/>
      <source>Restrict X</source>
      <translation>Ograničeno X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="116"/>
      <source>Restrict Y</source>
      <translation>Ograničeno Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="121"/>
      <source>Restrict Z</source>
      <translation>Ograničeno Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="126"/>
      <location filename="../../DraftGui.py" line="796"/>
      <source>Select edge</source>
      <translation>Odaberite rub</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="131"/>
      <source>Add custom snap point</source>
      <translation>Dodaje prilagođenu "privuci na" točku</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="136"/>
      <source>Length mode</source>
      <translation>Dužina mod</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="141"/>
      <location filename="../../DraftGui.py" line="792"/>
      <source>Wipe</source>
      <translation>Obriši</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="146"/>
      <source>Set Working Plane</source>
      <translation>Odaberite Radnu Ravninu</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="151"/>
      <source>Cycle snap object</source>
      <translation>Kružni Privuci objekt</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="156"/>
      <source>Toggle near snap on/off</source>
      <translation>uključi / isključi privuci u blizini </translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="330"/>
      <source>Draft Command Bar</source>
      <translation>Naredbene trake nacrta</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="659"/>
      <location filename="../../WorkingPlane.py" line="821"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="374"/>
      <source>Top</source>
      <translation>Gore</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="661"/>
      <location filename="../../WorkingPlane.py" line="832"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="386"/>
      <source>Front</source>
      <translation>Ispred</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="663"/>
      <location filename="../../WorkingPlane.py" line="843"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="398"/>
      <source>Side</source>
      <translation>Strana</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="665"/>
      <source>Auto</source>
      <translation>Automatski</translation>
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
      <translation>Prazno</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="728"/>
      <source>active command:</source>
      <translation>aktivna naredba:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="730"/>
      <source>Active Draft command</source>
      <translation>Aktivna naredbu nacrta</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="731"/>
      <source>X coordinate of next point</source>
      <translation>X koordinate sljedeće točke</translation>
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
      <translation>Y koordinate sljedeće točke</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="736"/>
      <source>Z coordinate of next point</source>
      <translation>Z koordinate sljedeće točke</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="737"/>
      <source>Enter point</source>
      <translation>Unesi točku</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="739"/>
      <source>Enter a new point with the given coordinates</source>
      <translation>Unesite novu točku s danim koordinatama</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="740"/>
      <source>Length</source>
      <translation>Dužina</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="741"/>
      <location filename="../../draftguitools/gui_trimex.py" line="220"/>
      <source>Angle</source>
      <translation>Kut</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="742"/>
      <source>Length of current segment</source>
      <translation>Dužina aktualnog odjeljka</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="743"/>
      <source>Angle of current segment</source>
      <translation>Kut aktualnog odjeljka</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="747"/>
      <source>Check this to lock the current angle</source>
      <translation>Uključi za zaključavanje aktualnog kuta</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="748"/>
      <location filename="../../DraftGui.py" line="1108"/>
      <source>Radius</source>
      <translation>Radijus</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="749"/>
      <location filename="../../DraftGui.py" line="1109"/>
      <source>Radius of Circle</source>
      <translation>Radijus kruga</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="754"/>
      <source>Coordinates relative to last point or to coordinate system origin
if is the first point to set</source>
      <translation>Koordinate u relativnom odnosu na posljednju točku ili ishodište koordinatnog sustava
ako je prva točka za postavljanje</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="759"/>
      <source>Coordinates relative to global coordinate system.
Uncheck to use working plane coordinate system</source>
      <translation>Koordinate u relativnom odnosu na globalni koordinatni sustav.
Poništite odabir, za korištenje koordinatnog sustava radne ravnine</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="761"/>
      <source>Filled</source>
      <translation>Ispunjeno</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="765"/>
      <source>Check this if the object should appear as filled, otherwise it will appear as wireframe.
Not available if Draft preference option 'Use Part Primitives' is enabled</source>
      <translation>Označite ako se objekt treba pojaviti kao ispunjen, jer će se u protivnom pojaviti kao žičana mreža.
Nije dostupno ako je omogućena opcija 'Koristi se primitivni dio' postavki Nacrta </translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="767"/>
      <source>Finish</source>
      <translation>Završiti</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="769"/>
      <source>Finishes the current drawing or editing operation</source>
      <translation>Završava aktualni crtež ili uređuje operacije</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="772"/>
      <source>If checked, command will not finish until you press the command button again</source>
      <translation>Ako je označeno, naredba neće završiti sve dok se ponovno ne pritisnete gumb naredbe</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="777"/>
      <source>If checked, an OCC-style offset will be performedinstead of the classic offset</source>
      <translation>Ako je označeno, umjesto klasičnog pomaka izvodit će se pomak u OCC stilu</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="778"/>
      <source>&amp;OCC-style offset</source>
      <translation>pomak u &amp;OCC-stilu</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="788"/>
      <source>&amp;Undo (CTRL+Z)</source>
      <translation>&amp;Poništi (CTRL + Z)</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="789"/>
      <source>Undo the last segment</source>
      <translation>Poništiti posljednji segment</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="791"/>
      <source>Finishes and closes the current line</source>
      <translation>Završi i zatvori aktualnu liniju</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="793"/>
      <source>Wipes the existing segments of this line and starts again from the last point</source>
      <translation>Briše postojeće segmente ovog retka i počinje ponovo od zadnje točke</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="794"/>
      <source>Set WP</source>
      <translation>Postavljanje WP</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="795"/>
      <source>Reorients the working plane on the last segment</source>
      <translation>Preusmjerava radnu ravninu na posljednji segment</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="797"/>
      <source>Selects an existing edge to be measured by this dimension</source>
      <translation>Odabire postojeći rub za mjerenje sa ovom dimenzijom</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="798"/>
      <source>Sides</source>
      <translation>Strane</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="799"/>
      <source>Number of sides</source>
      <translation>Broj strana</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="802"/>
      <source>If checked, objects will be copied instead of moved. Preferences -&gt; Draft -&gt; Global copy mode to keep this mode in next commands</source>
      <translation>Ako je označeno, objekti će se kopirati umjesto premještanja. Postavke-&gt; Nacrt-&gt; Globalno kopiranje mod da bi zadržao ovaj mod u sljedećim naredbama</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="803"/>
      <source>Modify subelements</source>
      <translation>Izmjene pod-elemenata</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="804"/>
      <source>If checked, subelements will be modified instead of entire objects</source>
      <translation>Ako je označeno, pod-elementi će se mijenjati umjesto cijelih objekata</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="805"/>
      <source>Text string to draw</source>
      <translation>Tekst za crtanje</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="806"/>
      <source>String</source>
      <translation>Tekst (string)</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="807"/>
      <source>Height of text</source>
      <translation>Visina teksta</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="808"/>
      <source>Height</source>
      <translation>Visina</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="809"/>
      <source>Intercharacter spacing</source>
      <translation>Unutrašnji razmak između znakova</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="810"/>
      <source>Tracking</source>
      <translation>Praćenje</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="811"/>
      <source>Full path to font file:</source>
      <translation>Cijeli put do datoteke pisma:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="812"/>
      <source>Open a FileChooser for font file</source>
      <translation>Otvorite FileChooser za datoteku fonta</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="813"/>
      <source>Create text</source>
      <translation>Napravi tekst</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="814"/>
      <source>Press this button to create the text object, or finish your text with two blank lines</source>
      <translation>Pritisnite ovaj gumb da biste stvorili tekstualni objekt ili dovršite tekst s dva prazna retka</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="836"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="272"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="327"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="530"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="541"/>
      <source>Current working plane</source>
      <translation>Aktualna radna ravnina</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="837"/>
      <source>Change default style for new objects</source>
      <translation>Promijenite zadani stil za nove objekte

</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="838"/>
      <source>Toggle construction mode</source>
      <translation>Uključivanje/isključivanje konstrukcijskog moda</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="839"/>
      <location filename="../../DraftGui.py" line="2050"/>
      <location filename="../../DraftGui.py" line="2065"/>
      <source>Autogroup off</source>
      <translation>Auto grupa isključena</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="950"/>
      <source>Line</source>
      <translation>Linija</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="958"/>
      <source>DWire</source>
      <translation>DWire</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="976"/>
      <source>Circle</source>
      <translation>Krug</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="981"/>
      <source>Arc</source>
      <translation>Luk</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="986"/>
      <location filename="../../draftguitools/gui_rotate.py" line="286"/>
      <source>Rotate</source>
      <translation>Rotiraj</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="990"/>
      <source>Point</source>
      <translation>Točka</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1018"/>
      <source>Label</source>
      <translation>Oznaka</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1036"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="527"/>
      <location filename="../../draftguitools/gui_offset.py" line="243"/>
      <location filename="../../draftguitools/gui_offset.py" line="260"/>
      <location filename="../../draftguitools/gui_offset.py" line="324"/>
      <source>Offset</source>
      <translation>Pomak</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1042"/>
      <location filename="../../DraftGui.py" line="1100"/>
      <location filename="../../draftguitools/gui_trimex.py" line="215"/>
      <source>Distance</source>
      <translation>Udaljenost</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1043"/>
      <location filename="../../DraftGui.py" line="1101"/>
      <location filename="../../draftguitools/gui_trimex.py" line="217"/>
      <source>Offset distance</source>
      <translation>Udaljenost pomaka</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1097"/>
      <source>Trimex</source>
      <translation>Produži</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1197"/>
      <source>Pick Object</source>
      <translation>Odaberite objekt</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1203"/>
      <source>Edit</source>
      <translation>Uredi</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1253"/>
      <source>Local u0394X</source>
      <translation>Local u0394X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1254"/>
      <source>Local u0394Y</source>
      <translation>Local u0394Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1255"/>
      <source>Local u0394Z</source>
      <translation>Local u0394Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1257"/>
      <source>Local X</source>
      <translation>Lokalno X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1258"/>
      <source>Local Y</source>
      <translation>Lokalno Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1259"/>
      <source>Local Z</source>
      <translation>Lokalno Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1261"/>
      <source>Global u0394X</source>
      <translation>Global u0394X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1262"/>
      <source>Global u0394Y</source>
      <translation>Global u0394Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1263"/>
      <source>Global u0394Z</source>
      <translation>Global u0394Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1265"/>
      <source>Global X</source>
      <translation>Globalno X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1266"/>
      <source>Global Y</source>
      <translation>Globalno Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1267"/>
      <source>Global Z</source>
      <translation>Globalno Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1503"/>
      <source>Invalid Size value. Using 200.0.</source>
      <translation>Pogrešna veličina vrijednosti. Koristit će se 200.0.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1511"/>
      <source>Invalid Tracking value. Using 0.</source>
      <translation>Pogrešna veličina vrijednosti. Koristit će se 0.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1525"/>
      <source>Please enter a text string.</source>
      <translation>Molimo vas, unesite tekst poruke.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1534"/>
      <source>Select a Font file</source>
      <translation>Odaberite datoteku pisma</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1567"/>
      <source>Please enter a font file.</source>
      <translation>Molim unesite datoteku pisma.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2058"/>
      <source>Autogroup:</source>
      <translation>Automatska grupa:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2394"/>
      <source>Faces</source>
      <translation>Plohe</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2395"/>
      <source>Remove</source>
      <translation>Ukloniti</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2396"/>
      <source>Add</source>
      <translation>Dodaj</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2397"/>
      <source>Facebinder elements</source>
      <translation>Povezana-lica elementi</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="46"/>
      <source>Draft</source>
      <translation>Skica</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="209"/>
      <location filename="../../importDWG.py" line="281"/>
      <source>LibreDWG error</source>
      <translation>LibreDWG greška</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="218"/>
      <location filename="../../importDWG.py" line="290"/>
      <source>Converting:</source>
      <translation>Pretvaranje:</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="223"/>
      <source>Conversion successful</source>
      <translation>Pretvorba je uspješna</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="226"/>
      <source>Error during DWG conversion. Try moving the DWG file to a directory path without spaces and non-english characters, or try saving to a lower DWG version.</source>
      <translation>Pogreška tijekom DWG pretvorbe. Pokušajte premjestiti DWG datoteku na put direktorija bez razmaka i neengleskih znakova ili pokušajte spremiti u nižu DWG verziju.</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="229"/>
      <location filename="../../importDWG.py" line="296"/>
      <source>ODA File Converter not found</source>
      <translation>ODA pretvarač datoteka nije pronađen</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="242"/>
      <location filename="../../importDWG.py" line="306"/>
      <source>QCAD error</source>
      <translation>QCAD greška</translation>
    </message>
    <message>
      <location filename="../../WorkingPlane.py" line="713"/>
      <location filename="../../draftmake/make_sketch.py" line="127"/>
      <location filename="../../draftmake/make_sketch.py" line="139"/>
      <source>All Shapes must be coplanar</source>
      <translation>Svi oblici moraju biti komplanarni</translation>
    </message>
    <message>
      <location filename="../../WorkingPlane.py" line="721"/>
      <source>Selected Shapes must define a plane</source>
      <translation>Odabrani oblici moraju definirati ravninu</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="81"/>
      <source>No graphical interface</source>
      <translation>Nema grafičkog sučelja</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="161"/>
      <source>Unable to insert new object into a scaled part</source>
      <translation>Nije moguće umetnuti novi objekt u skalirani dio</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="267"/>
      <source>Symbol not implemented. Using a default symbol.</source>
      <translation>Simbol nije implementiran. Korištenje zadanog simbola.</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="333"/>
      <source>Visibility off; removed from list: </source>
      <translation>Vidljivost isključena; uklonjeno s popisa:</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="603"/>
      <source>image is Null</source>
      <translation>slika je Ništavna</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="609"/>
      <source>filename does not exist on the system or in the resource file</source>
      <translation>naziv datoteke ne postoji na sustavu ili u datoteci resursa</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="668"/>
      <source>unable to load texture</source>
      <translation>nije moguće učitati teksturu</translation>
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
      <translation>Nema aktivnog dokumenta. Prekid.</translation>
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
      <translation>Pogrešan unos: objekt nije u dokumentu.</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="738"/>
      <source>Does not have 'ViewObject.RootNode'.</source>
      <translation>Nema 'ViewObject.RootNode'.</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="51"/>
      <location filename="../../draftutils/init_draft_statusbar.py" line="58"/>
      <location filename="../../draftutils/init_draft_statusbar.py" line="65"/>
      <location filename="../../draftutils/init_draft_statusbar.py" line="154"/>
      <source>custom</source>
      <translation>prilagođeno</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="140"/>
      <source>Unable to convert input into a  scale factor</source>
      <translation type="unfinished">Unable to convert input into a  scale factor</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="155"/>
      <source>Set custom scale</source>
      <translation type="unfinished">Set custom scale</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="157"/>
      <source>Set custom annotation scale in format x:x, x=x</source>
      <translation>Postavite prilagođenu skalu napomena u formatu x:x, x=x</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="214"/>
      <source>Set the scale used by draft annotation tools</source>
      <translation type="unfinished">Set the scale used by draft annotation tools</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="650"/>
      <source>Solids:</source>
      <translation>Čvrsta tijela:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="651"/>
      <source>Faces:</source>
      <translation>Plohe:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="652"/>
      <source>Wires:</source>
      <translation>Žice:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="653"/>
      <source>Edges:</source>
      <translation>Rubovi:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="654"/>
      <source>Vertices:</source>
      <translation>Vrhovi:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="658"/>
      <source>Face</source>
      <translation>Površina</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="663"/>
      <source>Wire</source>
      <translation>Žica</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="695"/>
      <location filename="../../draftutils/utils.py" line="699"/>
      <source>different types</source>
      <translation>različiti tipovi</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="709"/>
      <source>Objects have different placements. Distance between the two base points: </source>
      <translation>Objekti imaju različite položaje. Udaljenost između dvije osnovne točke:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="712"/>
      <source>has a different value</source>
      <translation>ima drugačiju vrijednost</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="715"/>
      <source>doesn't exist in one of the objects</source>
      <translation>ne postoji u jednom od objekata</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="827"/>
      <source>%s shares a base with %d other objects. Please check if you want to modify this.</source>
      <translation>%s dijeli bazu s %d drugim objektima. Molimo provjerite želite li ovo izmijeniti.</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="833"/>
      <source>%s cannot be modified because its placement is readonly.</source>
      <translation>%s nije moguće izmijeniti jer je postavljeno samo za čitanje.</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="977"/>
      <source>Wrong input: unknown document.</source>
      <translation>Pogrešan unos: nepoznat dokument.</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="1055"/>
      <source>This function will be deprecated in </source>
      <translation>Ova će funkcija biti zastarjela u</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="1056"/>
      <location filename="../../draftutils/utils.py" line="1059"/>
      <source>Please use </source>
      <translation>Molimo koristite</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="1059"/>
      <source>This function will be deprecated. </source>
      <translation>Ova će funkcija biti zastarjela </translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="169"/>
      <source>Snap Lock</source>
      <translation type="unfinished">Snap Lock</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="170"/>
      <source>Snap Endpoint</source>
      <translation type="unfinished">Snap Endpoint</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="171"/>
      <source>Snap Midpoint</source>
      <translation type="unfinished">Snap Midpoint</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="172"/>
      <source>Snap Center</source>
      <translation type="unfinished">Snap Center</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="173"/>
      <source>Snap Angle</source>
      <translation type="unfinished">Snap Angle</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="174"/>
      <source>Snap Intersection</source>
      <translation type="unfinished">Snap Intersection</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="175"/>
      <source>Snap Perpendicular</source>
      <translation type="unfinished">Snap Perpendicular</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="176"/>
      <source>Snap Extension</source>
      <translation type="unfinished">Snap Extension</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="177"/>
      <source>Snap Parallel</source>
      <translation type="unfinished">Snap Parallel</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="178"/>
      <source>Snap Special</source>
      <translation type="unfinished">Snap Special</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="179"/>
      <source>Snap Near</source>
      <translation type="unfinished">Snap Near</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="180"/>
      <source>Snap Ortho</source>
      <translation type="unfinished">Snap Ortho</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="181"/>
      <source>Snap Grid</source>
      <translation type="unfinished">Snap Grid</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="182"/>
      <source>Snap WorkingPlane</source>
      <translation type="unfinished">Snap WorkingPlane</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="183"/>
      <source>Snap Dimensions</source>
      <translation type="unfinished">Snap Dimensions</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="187"/>
      <source>Toggle Draft Grid</source>
      <translation>Uključi/Isključi rešetku nacrta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="69"/>
      <source>ShapeString: string has no wires</source>
      <translation>Oblik-tekstom: slova su bez linija</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="89"/>
      <location filename="../../draftobjects/draft_annotation.py" line="105"/>
      <source>added view property 'ScaleMultiplier'</source>
      <translation>dodano svojstvo prikaza 'Multiplikator skale'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="125"/>
      <location filename="../../draftobjects/draft_annotation.py" line="130"/>
      <source>migrated 'DraftText' type to 'Text'</source>
      <translation>premještena vrsta 'Tekst Nacrta' u 'Tekst'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="284"/>
      <source>, path object doesn't have 'Edges'.</source>
      <translation>, objekt puta nema 'Rubove'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="395"/>
      <location filename="../../draftobjects/patharray.py" line="401"/>
      <location filename="../../draftobjects/patharray.py" line="407"/>
      <source>'PathObj' property will be migrated to 'PathObject'</source>
      <translation>Svojstvo 'PathObj' bit će premješteno u 'PathObject'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="525"/>
      <source>Cannot calculate path tangent. Copy not aligned.</source>
      <translation>Nije moguće izračunati tangentu putanje. Kopija nije poravnata.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="541"/>
      <source>Tangent and normal are parallel. Copy not aligned.</source>
      <translation>Tangenta i normala su paralelne. Kopija nije poravnata.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="558"/>
      <source>Cannot calculate path normal, using default.</source>
      <translation>Nije moguće izračunati  put normale, koristit će se zadano.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="565"/>
      <source>Cannot calculate path binormal. Copy not aligned.</source>
      <translation>Ne može se izračunati binormalni put. Kopija nije poravnata.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="571"/>
      <source>AlignMode {} is not implemented</source>
      <translation>PoravnanjeMod {} nije implementiran
</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="145"/>
      <location filename="../../draftobjects/pointarray.py" line="161"/>
      <source>added property 'ExtraPlacement'</source>
      <translation>dodano svojstvo 'DodatnoPostavljanje'</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="151"/>
      <source>Object must be a closed shape</source>
      <translation>Objekt mora biti zatvoreni oblik</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="153"/>
      <source>No solid object created</source>
      <translation>Nije stvoreno čvrsto tijelo</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="276"/>
      <source>Faces must be coplanar to be refined</source>
      <translation>Lica moraju biti komplanarna da bi se izgladila</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="435"/>
      <location filename="../../draftfunctions/downgrade.py" line="230"/>
      <source>Upgrade: Unknown force method:</source>
      <translation>Nadogradnja: Nepoznata metoda prisile:</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="453"/>
      <source>Found groups: closing each open object inside</source>
      <translation>Pronađene grupe: zatvara svaki otvoreni objekt unutar</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="459"/>
      <source>Found meshes: turning into Part shapes</source>
      <translation>Pronađene mreže: pretvaranje u oblike djelova</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="467"/>
      <source>Found 1 solidifiable object: solidifying it</source>
      <translation>Pronađen 1 objekt za učvršćivanje: napravi čvrsto tijelo</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="472"/>
      <source>Found 2 objects: fusing them</source>
      <translation>Pronađena 2 objekta: spoji</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="483"/>
      <source>Found object with several coplanar faces: refine them</source>
      <translation>Pronađen objekt s nekoliko komplanarnih lica: pročistite ih
</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="489"/>
      <source>Found 1 non-parametric objects: draftifying it</source>
      <translation>Pronađen je 1 ne parametarski objekt: napravi parametarski objekt</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="500"/>
      <source>Found 1 closed sketch object: creating a face from it</source>
      <translation>Pronađen 1 zatvoreni objekt skice: stvaranje lice od njega</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="505"/>
      <source>Found closed wires: creating faces</source>
      <translation>Pronađene zatvorene žice: stvaranje lica</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="511"/>
      <source>Found several wires or edges: wiring them</source>
      <translation>Pronašli smo nekoliko žica ili rubova: poveži ih</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="513"/>
      <location filename="../../draftfunctions/upgrade.py" line="547"/>
      <source>Found several non-treatable objects: creating compound</source>
      <translation>Pronađeno je nekoliko objekata koji su nepopravljivi: stvaranje spoja</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="518"/>
      <source>trying: closing it</source>
      <translation>pokušava: zatvoriti to</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="520"/>
      <source>Found 1 open wire: closing it</source>
      <translation>Pronađena 1 otvorena žica: spajanje</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="537"/>
      <source>Found 1 object: draftifying it</source>
      <translation>Pronađen 1 objekt: nadogradi ga</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="542"/>
      <source>Found points: creating compound</source>
      <translation>Pronađene točke: stvori složen spoj</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="550"/>
      <source>Unable to upgrade these objects.</source>
      <translation>Nije moguće ažurirati ove objekte.</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="90"/>
      <source>No object given</source>
      <translation>Nema danog objekta</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="94"/>
      <source>The two points are coincident</source>
      <translation>Dvije točke se podudaraju</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="113"/>
      <source>mirrored</source>
      <translation>zrcaljen</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="238"/>
      <source>Found 1 block: exploding it</source>
      <translation>Pronađeno 1 blok: razdjeli ga</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="246"/>
      <source>Found 1 multi-solids compound: exploding it</source>
      <translation>Pronađeno 1 složeno višestruko čvrsto tijelo: razdjeli ga</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="253"/>
      <source>Found 1 parametric object: breaking its dependencies</source>
      <translation>Pronađen 1 parametarski objekt: uklanjanje njegovih ovisnosti</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="261"/>
      <source>Found 2 objects: subtracting them</source>
      <translation>Pronađena 2 objekta: oduzmi ih</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="268"/>
      <source>Found several faces: splitting them</source>
      <translation>Pronađeno nekoliko lica: razdjeli ih</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="273"/>
      <source>Found several objects: subtracting them from the first one</source>
      <translation>Pronađeno nekoliko objekata: oduzimanje tih objekata od prvog objekta</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="278"/>
      <source>Found 1 face: extracting its wires</source>
      <translation>Pronađeno 1 lice: izdvajanje njegovih žica</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="284"/>
      <source>Found only wires: extracting their edges</source>
      <translation>Našao samo žice: izdvajanje njihovih rubova</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="288"/>
      <source>No more downgrade possible</source>
      <translation>Nije moguće vraćanje na stariju verziju</translation>
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
      <translation>Pogrešan unos: mora biti vektor.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_pointarray.py" line="147"/>
      <location filename="../../draftmake/make_text.py" line="107"/>
      <location filename="../../draftmake/make_label.py" line="215"/>
      <source>Wrong input: must be a placement, a vector, or a rotation.</source>
      <translation>Pogrešan unos: mora biti položaj, vektor ili rotacija.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="316"/>
      <location filename="../../draftmake/make_label.py" line="230"/>
      <source>Wrong input: object must not be a list.</source>
      <translation>Pogrešan unos: objekt ne smije biti popis.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_patharray.py" line="213"/>
      <location filename="../../draftmake/make_label.py" line="251"/>
      <source>Wrong input: must be a list or tuple of strings, or a single string.</source>
      <translation>Pogrešan unos: mora biti popis ili niz riječi ili jedna riječ.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="263"/>
      <source>Wrong input: subelement not in object.</source>
      <translation>Pogrešan unos: pod-element nije u objektu.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="272"/>
      <source>Wrong input: label_type must be a string.</source>
      <translation>Pogrešan unos: tip oznake mora biti slova.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="277"/>
      <source>Wrong input: label_type must be one of the following: </source>
      <translation>Pogrešan unos: ip oznake mora biti jedno od sljedećeg:

</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_text.py" line="91"/>
      <location filename="../../draftmake/make_text.py" line="96"/>
      <location filename="../../draftmake/make_label.py" line="286"/>
      <location filename="../../draftmake/make_label.py" line="291"/>
      <source>Wrong input: must be a list of strings or a single string.</source>
      <translation>Pogrešan unos: mora biti popis znakova ili jedan znak.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="300"/>
      <location filename="../../draftmake/make_label.py" line="304"/>
      <source>Wrong input: must be a string, 'Horizontal', 'Vertical', or 'Custom'.</source>
      <translation>Pogrešan unos: mora biti slova, "Horizontalno", "Okomito" ili "Prilagođeno".</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_polararray.py" line="119"/>
      <location filename="../../draftmake/make_layer.py" line="201"/>
      <location filename="../../draftmake/make_patharray.py" line="191"/>
      <location filename="../../draftmake/make_patharray.py" line="360"/>
      <location filename="../../draftmake/make_orthoarray.py" line="151"/>
      <location filename="../../draftmake/make_label.py" line="313"/>
      <source>Wrong input: must be a number.</source>
      <translation>Pogrešan unos: mora bit broj.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="320"/>
      <source>Wrong input: must be a list of at least two vectors.</source>
      <translation>Pogrešan unos: mora biti popis od najmanje dva vektora.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="353"/>
      <source>Direction is not 'Custom'; points won't be used.</source>
      <translation>Smjer nije 'Prilagođeno'; točke se neće koristiti.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="380"/>
      <source>Wrong input: must be a list of two elements. For example, [object, 'Edge1'].</source>
      <translation>Pogrešan unos: mora biti popis od dva elementa. Na primjer, [objekt, 'Rub1'].</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_pointarray.py" line="135"/>
      <source>Wrong input: point object doesn't have 'Geometry', 'Links', or 'Components'.</source>
      <translation>Pogrešan unos: točkasti objekt nema 'Geometrija', 'Poveznice' ili 'Komponente'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="69"/>
      <source>Layers</source>
      <translation>Slojevi</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="145"/>
      <location filename="../../draftmake/make_layer.py" line="162"/>
      <location filename="../../draftguitools/gui_layers.py" line="47"/>
      <source>Layer</source>
      <translation>Sloj</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="157"/>
      <source>Wrong input: it must be a string.</source>
      <translation>Pogrešan unos: mora biti slova.

</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="167"/>
      <location filename="../../draftmake/make_layer.py" line="171"/>
      <location filename="../../draftmake/make_layer.py" line="184"/>
      <location filename="../../draftmake/make_layer.py" line="188"/>
      <source>Wrong input: must be a tuple of three floats 0.0 to 1.0.</source>
      <translation>Pogrešan unos: mora biti skup od tri decimale od 0,0 do 1,0.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="208"/>
      <location filename="../../draftmake/make_layer.py" line="212"/>
      <source>Wrong input: must be 'Solid', 'Dashed', 'Dotted', or 'Dashdot'.</source>
      <translation>Pogrešan unos: mora biti 'Čvrsto tijelo', 'Iscrtkano', 'Istočkano' ili 'CrticaTočka'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="220"/>
      <source>Wrong input: must be a number between 0 and 100.</source>
      <translation>Pogrešan unos: mora biti broj između 0 i 100.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="173"/>
      <source>This function is deprecated. Do not use this function directly.</source>
      <translation>Ova funkcija je zastarjela. Ne koristite ovu funkciju izravno.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="174"/>
      <source>Use one of 'make_linear_dimension', or 'make_linear_dimension_obj'.</source>
      <translation>Koristite jednu od 'make_linear_dimension' ili 'make_linear_dimension_obj'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="327"/>
      <location filename="../../draftmake/make_dimension.py" line="452"/>
      <source>Wrong input: object doesn't have a 'Shape' to measure.</source>
      <translation>Pogrešan unos: objekt nema 'Oblik' za mjerenje.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="331"/>
      <source>Wrong input: object doesn't have at least one element in 'Vertexes' to use for measuring.</source>
      <translation>Pogrešan unos: objekt nema barem jedan element u 'Rubovima' za korištenje kod mjerenja.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="338"/>
      <location filename="../../draftmake/make_dimension.py" line="463"/>
      <source>Wrong input: must be an integer.</source>
      <translation>Pogrešan unos: mora biti cijeli broj.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="343"/>
      <source>i1: values below 1 are not allowed; will be set to 1.</source>
      <translation>i1: vrijednosti ispod 1 nisu dopuštene; bit će postavljeno na 1.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="347"/>
      <location filename="../../draftmake/make_dimension.py" line="363"/>
      <source>Wrong input: vertex not in object.</source>
      <translation>Pogrešan unos: vrh nije u objektu.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="359"/>
      <source>i2: values below 1 are not allowed; will be set to the last vertex in the object.</source>
      <translation>i2: vrijednosti ispod 1 nisu dopuštene; bit će postavljeno na posljednji vrh u objektu.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="456"/>
      <source>Wrong input: object doesn't have at least one element in 'Edges' to use for measuring.</source>
      <translation>Pogrešan unos: objekt nema barem jedan element u 'Rubovima' za korištenje kod mjerenja.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="468"/>
      <source>index: values below 1 are not allowed; will be set to 1.</source>
      <translation>indeks: vrijednosti ispod 1 nisu dopuštene; bit će postavljeno na 1.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="472"/>
      <source>Wrong input: index doesn't correspond to an edge in the object.</source>
      <translation>Pogrešan unos: indeks ne odgovara rubu u objektu.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="476"/>
      <source>Wrong input: index doesn't correspond to a circular edge.</source>
      <translation>Pogrešan unos: indeks ne odgovara kružnom rubu.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="483"/>
      <location filename="../../draftmake/make_dimension.py" line="487"/>
      <source>Wrong input: must be a string, 'radius' or 'diameter'.</source>
      <translation>Pogrešan unos: mora biti znak, 'polumjer' ili 'promjer'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="579"/>
      <location filename="../../draftmake/make_dimension.py" line="586"/>
      <source>Wrong input: must be a list with two angles.</source>
      <translation>Pogrešan unos: mora biti popis s dva kuta.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="60"/>
      <source>Internal orthogonal array</source>
      <translation>Unutarnja ortogonalna matrica</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="87"/>
      <source>Wrong input: must be a number or vector.</source>
      <translation>Pogrešan unos: mora biti broj ili vektor.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="92"/>
      <location filename="../../draftmake/make_orthoarray.py" line="95"/>
      <location filename="../../draftmake/make_orthoarray.py" line="98"/>
      <source>Input: single value expanded to vector.</source>
      <translation>Ulaz: jedna vrijednost proširena na vektor.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="154"/>
      <location filename="../../draftmake/make_polararray.py" line="112"/>
      <location filename="../../draftmake/make_orthoarray.py" line="119"/>
      <source>Wrong input: must be an integer number.</source>
      <translation>Pogrešan unos: mora biti cijeli broj.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="123"/>
      <location filename="../../draftmake/make_orthoarray.py" line="126"/>
      <location filename="../../draftmake/make_orthoarray.py" line="129"/>
      <source>Input: number of elements must be at least 1. It is set to 1.</source>
      <translation>Ulaz: broj elemenata mora biti najmanje 1. Postavljen je na 1.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="275"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="269"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="396"/>
      <source>Orthogonal array</source>
      <translation>Ortogonalna matrica</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="350"/>
      <source>Orthogonal array 2D</source>
      <translation>Ortogonalna 2D matrica</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="424"/>
      <source>Rectangular array</source>
      <translation>Pravokutna matrica.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="501"/>
      <source>Rectangular array 2D</source>
      <translation>Pravokutna 2D matrica</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_polararray.py" line="94"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="258"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="434"/>
      <source>Polar array</source>
      <translation>Polarna matrica</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_patharray.py" line="246"/>
      <source>Wrong input: must be 'Original', 'Frenet', or 'Tangent'.</source>
      <translation>Pogrešan unos: mora biti "Original", "Frenet" ili "Tangencial".</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="125"/>
      <location filename="../../draftmake/make_arc_3points.py" line="130"/>
      <source>Points:</source>
      <translation>Točke:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="126"/>
      <location filename="../../draftmake/make_arc_3points.py" line="131"/>
      <source>Wrong input: must be list or tuple of three points exactly.</source>
      <translation>Pogrešan unos: mora biti popis ili skup od tri točke točno.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="138"/>
      <source>Placement:</source>
      <translation>Postavljanje:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="139"/>
      <source>Wrong input: incorrect type of placement.</source>
      <translation>Pogrešan unos: netočna vrsta položaja.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="153"/>
      <source>Wrong input: incorrect type of points.</source>
      <translation>Pogrešan unos: netočna vrsta točaka.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="159"/>
      <source>Cannot generate shape:</source>
      <translation>Nije moguće generirati oblik:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="166"/>
      <source>Radius:</source>
      <translation>Polumjer:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="167"/>
      <source>Center:</source>
      <translation>Centar:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="170"/>
      <source>Create primitive object</source>
      <translation>Stvori jednostavni objekt</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="193"/>
      <location filename="../../draftmake/make_arc_3points.py" line="204"/>
      <source>Final placement:</source>
      <translation>Završno postavljanje:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="195"/>
      <source>Face: True</source>
      <translation>Lice: Istinito</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="197"/>
      <source>Support:</source>
      <translation>Potpora:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="198"/>
      <source>Map mode:</source>
      <translation>Način karte:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_sketch.py" line="104"/>
      <source>No shape found</source>
      <translation>Nisam našao oblik</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_sketch.py" line="111"/>
      <source>All Shapes must be planar</source>
      <translation>Svi oblici moraju biti ravan</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="122"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="290"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="481"/>
      <source>Circular array</source>
      <translation>Kružna matrica</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="144"/>
      <source>Wrong input: must be a number or quantity.</source>
      <translation>Pogrešan unos: mora biti broj ili količina.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="58"/>
      <location filename="../../draftmake/make_fillet.py" line="143"/>
      <location filename="../../draftmake/make_fillet.py" line="144"/>
      <location filename="../../draftmake/make_fillet.py" line="145"/>
      <source>length:</source>
      <translation>dužina:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="132"/>
      <source>Two elements are needed.</source>
      <translation>Potrebna su dva elementa.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="139"/>
      <source>Radius is too large</source>
      <translation>Polumjer je prevelik</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="143"/>
      <location filename="../../draftmake/make_fillet.py" line="144"/>
      <location filename="../../draftmake/make_fillet.py" line="145"/>
      <source>Segment</source>
      <translation>Segment</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="165"/>
      <source>Removed original objects.</source>
      <translation>Uklonjeni originalni objekti.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="87"/>
      <source>Select an object to scale</source>
      <translation>Odaberite objekt za skaliranje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="108"/>
      <source>Pick base point</source>
      <translation>Odaberite točku odredišta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="135"/>
      <source>Pick reference distance from base point</source>
      <translation>Odaberite referencu udaljenost od odredišne točke</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="206"/>
      <location filename="../../draftguitools/gui_scale.py" line="236"/>
      <location filename="../../draftguitools/gui_scale.py" line="359"/>
      <source>Scale</source>
      <translation>Skaliraj</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="209"/>
      <source>Some subelements could not be scaled.</source>
      <translation>Neke pod elemente nije bilo moguće skalirati.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="339"/>
      <source>Unable to scale object:</source>
      <translation>Nije moguće skalirati objekt:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="343"/>
      <source>Unable to scale objects:</source>
      <translation>Nije moguće skalirati objekte:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="346"/>
      <source>This object type cannot be scaled directly. Please use the clone method.</source>
      <translation>Ova vrsta objekta ne može se izravno skalirati. Molimo koristite metodu kloniranja.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="407"/>
      <source>Pick new distance from base point</source>
      <translation>Odaberite novu udaljenost od bazne točke</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_sketcher_objects.py" line="63"/>
      <source>Sketch is too complex to edit: it is suggested to use sketcher default editor</source>
      <translation>Skica je previše složena za uređivanje: predlaže se korištenje zadanog uređivača skice</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="80"/>
      <source>Pick target point</source>
      <translation>Odaberite točku odredišta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="157"/>
      <source>Create Label</source>
      <translation>Stvori oznaku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="191"/>
      <location filename="../../draftguitools/gui_labels.py" line="218"/>
      <source>Pick endpoint of leader line</source>
      <translation>Odaberite krajnju točku od linije vođenja</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="201"/>
      <location filename="../../draftguitools/gui_labels.py" line="228"/>
      <source>Pick text position</source>
      <translation>Odaberite poziciju teksta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="75"/>
      <source>Change Style</source>
      <translation>Promjeni stil</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="77"/>
      <source>Pick location point</source>
      <translation>Odaberite točku položaja</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="121"/>
      <source>Create Text</source>
      <translation>Stvori tekst</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="51"/>
      <source>Toggle grid</source>
      <translation>Pokaži/Sakrij rešetku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="150"/>
      <source>Pick a face, 3 vertices or a WP Proxy to define the drawing plane</source>
      <translation>Odaberite lice, 3 vrha ili WP proxy za definiranje ravnine crtanja</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="274"/>
      <source>Working plane aligned to global placement of</source>
      <translation>Radna ravnina usklađena s globalnim položajem</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="523"/>
      <source>Dir</source>
      <translation>Smjer</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="539"/>
      <source>Custom</source>
      <translation>Prilagođeno</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="71"/>
      <source>Select faces from existing objects</source>
      <translation>Odaberite lica na postojećim objektima</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="73"/>
      <source>Select an object to mirror</source>
      <translation>Odaberite objekt za zrcaljenje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="92"/>
      <source>Pick start point of mirror line</source>
      <translation>Odaberite početnu točku od linije zrcaljenja</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="122"/>
      <source>Mirror</source>
      <translation>Zrcaljenje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="177"/>
      <location filename="../../draftguitools/gui_mirror.py" line="203"/>
      <source>Pick end point of mirror line</source>
      <translation>Odaberite krajnju točku od linije zrcaljenja</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="78"/>
      <location filename="../../draftguitools/gui_arcs.py" line="88"/>
      <source>Pick center point</source>
      <translation>Odaberite točku središta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="189"/>
      <location filename="../../draftguitools/gui_polygons.py" line="200"/>
      <location filename="../../draftguitools/gui_polygons.py" line="260"/>
      <location filename="../../draftguitools/gui_arcs.py" line="254"/>
      <location filename="../../draftguitools/gui_arcs.py" line="270"/>
      <location filename="../../draftguitools/gui_arcs.py" line="410"/>
      <source>Pick radius</source>
      <translation>Odaberite polumjer</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="277"/>
      <location filename="../../draftguitools/gui_arcs.py" line="278"/>
      <location filename="../../draftguitools/gui_arcs.py" line="446"/>
      <location filename="../../draftguitools/gui_arcs.py" line="447"/>
      <source>Start angle</source>
      <translation>Početni kut</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="283"/>
      <location filename="../../draftguitools/gui_arcs.py" line="452"/>
      <source>Pick start angle</source>
      <translation>Odaberite početni kut</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="285"/>
      <location filename="../../draftguitools/gui_arcs.py" line="286"/>
      <location filename="../../draftguitools/gui_arcs.py" line="454"/>
      <location filename="../../draftguitools/gui_arcs.py" line="455"/>
      <source>Aperture angle</source>
      <translation>Kut otvora</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="291"/>
      <source>Pick aperture</source>
      <translation>Odaberite otvor</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="317"/>
      <source>Create Circle (Part)</source>
      <translation>Napravite krug (dio)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="335"/>
      <source>Create Circle</source>
      <translation>Stvori krug</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="369"/>
      <source>Create Arc (Part)</source>
      <translation>Napravite luk (dio)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="389"/>
      <source>Create Arc</source>
      <translation>Stvori luk</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="466"/>
      <source>Pick aperture angle</source>
      <translation>Odaberite kut otvora</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="509"/>
      <location filename="../../draftguitools/gui_arcs.py" line="551"/>
      <source>Arc by 3 points</source>
      <translation>Luk iz 3 točke</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="71"/>
      <location filename="../../draftguitools/gui_lines.py" line="83"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="122"/>
      <location filename="../../draftguitools/gui_rectangles.py" line="69"/>
      <source>Pick first point</source>
      <translation>Odaberite prvu točku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="163"/>
      <source>Create Line</source>
      <translation>Napravi Liniju</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="185"/>
      <source>Create Wire</source>
      <translation>Stvaranje žice</translation>
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
      <translation>Odaberite slijedeću točku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="330"/>
      <source>Unable to create a Wire from selected objects</source>
      <translation>Nije moguće stvoriti žicu od odabranih objekata</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="352"/>
      <source>Convert to Wire</source>
      <translation>Pretvori u žicu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="85"/>
      <location filename="../../draftguitools/gui_shapestrings.py" line="98"/>
      <source>Pick ShapeString location point</source>
      <translation>Odaberite točku mjesta Tekst oblika</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="176"/>
      <location filename="../../draftguitools/gui_shapestrings.py" line="133"/>
      <source>Create ShapeString</source>
      <translation>Napravi OblikTeksta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="305"/>
      <source>Select a Draft object to edit</source>
      <translation>Odaberite Nacrt objekt za uređivanje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="558"/>
      <source>No edit point found for selected object</source>
      <translation>Nije pronađena točka uređivanja za odabrani objekt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="811"/>
      <source>Too many objects selected, max number set to:</source>
      <translation>Odabrano je previše objekata, maksimalni broj postavljen na:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="819"/>
      <source>: this object is not editable</source>
      <translation>: ovaj objekt nije moguće uređivati</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="76"/>
      <source>Select an object to join</source>
      <translation>Odaberite jedan objekt za spojiti</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="99"/>
      <source>Join lines</source>
      <translation>Spoji linije</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="110"/>
      <source>Selection:</source>
      <translation>Odabrano:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="64"/>
      <source>Change slope</source>
      <translation>Promjeni nagib</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="94"/>
      <source>Select objects to trim or extend</source>
      <translation>Odaberite objekte za skraćivanje ili produživanje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="173"/>
      <location filename="../../draftguitools/gui_offset.py" line="143"/>
      <source>Pick distance</source>
      <translation>Odaberite udaljenost</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="222"/>
      <source>Offset angle</source>
      <translation>Kut pomaka</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="483"/>
      <source>Unable to trim these objects, only Draft wires and arcs are supported.</source>
      <translation>Nije moguće skratiti te objekte, samo žice Nacrta i lukovi su podržani.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="488"/>
      <source>Unable to trim these objects, too many wires</source>
      <translation>Nije moguće skratiti te objekte, previše žica</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="505"/>
      <source>These objects don't intersect.</source>
      <translation>Ovi objekti se ne sijeku.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="508"/>
      <source>Too many intersection points.</source>
      <translation>Previše sjecišta.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="66"/>
      <source>Select an object to convert.</source>
      <translation>Odaberite objekt za konvertiranje.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="99"/>
      <source>Convert to Sketch</source>
      <translation>Pretvori u Skicu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="114"/>
      <source>Convert to Draft</source>
      <translation>Pretvori u Nacrt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="143"/>
      <source>Convert Draft/Sketch</source>
      <translation>Pretvori Nacrt/Skicu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="104"/>
      <source>Please select exactly two objects, the base object and the point object, before calling this command.</source>
      <translation>Molimo odaberite točno dva objekta, osnovni objekt i objekt točaka, prije pozivanja ove naredbe.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="122"/>
      <source>Point array</source>
      <translation>Matrica točaka</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="108"/>
      <source>Select an object to edit</source>
      <translation>Odaberite objekt za uređivanje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="79"/>
      <source>Select an object to clone</source>
      <translation>Odaberite objekt za kloniranje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="212"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="247"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="261"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="294"/>
      <source>Create Dimension</source>
      <translation>Stvori Dimenziju</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="309"/>
      <source>Create Dimension (radial)</source>
      <translation>Stvori dimenziju (radijalno)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="508"/>
      <source>Edge too short!</source>
      <translation>Rub je prekratak!</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="518"/>
      <source>Edges don't intersect!</source>
      <translation>Rubovi se ne križaju!</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="75"/>
      <source>Select an object to stretch</source>
      <translation>Odaberite objekt za rastezanje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="127"/>
      <source>Pick first point of selection rectangle</source>
      <translation>Izaberite prvu točku na izbornom pravokutniku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="164"/>
      <source>Pick opposite point of selection rectangle</source>
      <translation>Izaberite suprotnu točku na izbornom pravokutniku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="173"/>
      <source>Pick start point of displacement</source>
      <translation>Odaberite početnu točku premještanja</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="236"/>
      <source>Pick end point of displacement</source>
      <translation>Odaberite završnu točku premještanja</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="448"/>
      <source>Turning one Rectangle into a Wire</source>
      <translation>Prebaci jedan Pravokutnik u Žicu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="477"/>
      <source>Stretch</source>
      <translation>Razvuci</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="102"/>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="79"/>
      <source>Please select exactly two objects, the base object and the path object, before calling this command.</source>
      <translation>Molimo odaberite točno dva objekta, osnovni objekt i put objekt, prije pozivanja ove naredbe.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="101"/>
      <source>Path twisted array</source>
      <translation>Matrica zaokrenuta po putu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="132"/>
      <location filename="../../draftguitools/gui_beziers.py" line="332"/>
      <source>Bézier curve has been closed</source>
      <translation type="unfinished">Bézier curve has been closed</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="140"/>
      <location filename="../../draftguitools/gui_beziers.py" line="368"/>
      <location filename="../../draftguitools/gui_splines.py" line="131"/>
      <source>Last point has been removed</source>
      <translation>Zadnja točka je uklonjena</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="153"/>
      <location filename="../../draftguitools/gui_splines.py" line="147"/>
      <source>Pick next point, or finish (A) or close (O)</source>
      <translation>Odaberite slijedeću točku, ili završeno (A) ili zatvori (O)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="211"/>
      <location filename="../../draftguitools/gui_beziers.py" line="451"/>
      <source>Create BezCurve</source>
      <translation>Napravi Bezierovu krivulju</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="376"/>
      <source>Click and drag to define next knot</source>
      <translation>Kliknite i povucite za određivanje slijedećeg čvora</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="382"/>
      <source>Click and drag to define next knot, or finish (A) or close (O)</source>
      <translation>Kliknite i povucite za određivanje slijedećeg čvora, ili završi (A) ili zatvori (O)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snapper.py" line="1543"/>
      <source>(ON)</source>
      <translation type="unfinished">(ON)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snapper.py" line="1546"/>
      <source>(OFF)</source>
      <translation type="unfinished">(OFF)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="67"/>
      <location filename="../../draftguitools/gui_upgrade.py" line="67"/>
      <source>Select an object to upgrade</source>
      <translation>Odaberite objekt za nadogradnju</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="85"/>
      <source>Downgrade</source>
      <translation>Vraćanje na niži stupanj</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="143"/>
      <source>Path array</source>
      <translation>Matrica Puta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="120"/>
      <source>Spline has been closed</source>
      <translation>Kriva je zatvorena</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="183"/>
      <source>Create B-spline</source>
      <translation>Stvaranje B-spline</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="125"/>
      <source>Create Plane</source>
      <translation>Napravi ravninu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="142"/>
      <source>Create Rectangle</source>
      <translation>Napravi pravokutnik</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="196"/>
      <location filename="../../draftguitools/gui_rectangles.py" line="202"/>
      <source>Pick opposite point</source>
      <translation>Odaberite suprotnu točku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="76"/>
      <source>Fillet radius</source>
      <translation>Polumjer Obruba</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="79"/>
      <source>Radius of fillet</source>
      <translation>Polumjer od obruba</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="107"/>
      <source>Enter radius.</source>
      <translation>Unesite polumjer.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="126"/>
      <source>Delete original objects:</source>
      <translation>Obriši originalne objekte:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="131"/>
      <source>Chamfer mode:</source>
      <translation>Žlijeb način:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="148"/>
      <source>Two elements needed.</source>
      <translation>Dva elementa potrebna.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="155"/>
      <source>Test object</source>
      <translation>Test objekt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="156"/>
      <source>Test object removed</source>
      <translation>Test objekt uklonjen</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="158"/>
      <source>Fillet cannot be created</source>
      <translation>Nije moguće stvoriti obrub</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="188"/>
      <source>Create fillet</source>
      <translation>Napravi obrub</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="65"/>
      <source>Add to group</source>
      <translation>Dodaj u grupu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="68"/>
      <source>Ungroup</source>
      <translation>Razgrupiraj</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="70"/>
      <source>Add new group</source>
      <translation>Dodaj novu grupu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="159"/>
      <source>Select group</source>
      <translation>Odabir grupe</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="193"/>
      <source>No new selection. You must select non-empty groups or objects inside groups.</source>
      <translation>Nema novog odabira. Morate odabrati neprazne grupe ili objekte unutar grupa.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="203"/>
      <source>Autogroup</source>
      <translation>Automatska grupa</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="250"/>
      <source>Add new Layer</source>
      <translation>Dodaj novi Sloj</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="304"/>
      <source>Add to construction group</source>
      <translation>Dodaj u grupu konstrukcije</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="355"/>
      <source>Add a new group with a given name</source>
      <translation>Dodajte novu grupu s određenim imenom</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="383"/>
      <source>Add group</source>
      <translation>Dodaj grupu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="385"/>
      <source>Group name</source>
      <translation>Naziv grupe</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="392"/>
      <source>Group</source>
      <translation>Grupa</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="92"/>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="569"/>
      <source>This object does not support possible coincident points, please try again.</source>
      <translation>Ovaj objekt ne podržava moguće podudarne točke, pokušajte ponovno.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="164"/>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="608"/>
      <source>Active object must have more than two points/nodes</source>
      <translation>Aktivni objekt mora imati više od dvije točke/čvorišta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="722"/>
      <source>Selection is not a Knot</source>
      <translation>Odabrano nije čvor</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="749"/>
      <source>Endpoint of BezCurve can't be smoothed</source>
      <translation>Ishod Bezierove krivulje ne može biti izglađen</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="77"/>
      <source>The Drawing Workbench is obsolete since 0.17, consider using the TechDraw Workbench instead.</source>
      <translation>Radni stol za crtanje zastario je od 0.17, razmislite o korištenju TechDraw-a.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="81"/>
      <location filename="../../draftguitools/gui_shape2dview.py" line="68"/>
      <source>Select an object to project</source>
      <translation>Odaberite jedan objekt za projekt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="85"/>
      <source>Upgrade</source>
      <translation>Nadogradi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="126"/>
      <source>Main toggle snap</source>
      <translation>Osnovna prebaci tipka Hvatanja</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="157"/>
      <source>Midpoint snap</source>
      <translation>Hvatanje na Srednju točku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="187"/>
      <source>Perpendicular snap</source>
      <translation>Hvatanje na Okomito</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="217"/>
      <source>Grid snap</source>
      <translation>Skok na rešetku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="247"/>
      <source>Intersection snap</source>
      <translation>Hvatanje na Sjecište</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="277"/>
      <source>Parallel snap</source>
      <translation>Hvatanje na Paralelno</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="307"/>
      <source>Endpoint snap</source>
      <translation>Hvatanje na Krajnju točku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="338"/>
      <source>Angle snap (30 and 45 degrees)</source>
      <translation>Hvatanje na Kut  (30 i 45 stupnjeva)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="368"/>
      <source>Arc center snap</source>
      <translation>Hvatanje na središte Luka</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="398"/>
      <source>Edge extension snap</source>
      <translation>Hvatanje na Proširenje ruba</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="428"/>
      <source>Near snap</source>
      <translation>Hvatanje na Obližnje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="459"/>
      <source>Orthogonal snap</source>
      <translation>Hvatanje na Ortogonalno</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="489"/>
      <source>Special point snap</source>
      <translation>Hvatanje na Specijalnu točku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="520"/>
      <source>Dimension display</source>
      <translation>Prikaz dimenzije</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="553"/>
      <source>Working plane snap</source>
      <translation>Hvatanje na Radne ravnine</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="583"/>
      <source>Show snap toolbar</source>
      <translation>Prikaži alatnu traku Hvatanje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="81"/>
      <source>Select an object to move</source>
      <translation>Odaberite objekt za pomjeranje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="103"/>
      <source>Pick start point</source>
      <translation>Odaberite početnu točku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="162"/>
      <location filename="../../draftguitools/gui_move.py" line="308"/>
      <source>Pick end point</source>
      <translation>Odaberite krajnju točku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="210"/>
      <source>Move</source>
      <translation>Pomicanje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="213"/>
      <location filename="../../draftguitools/gui_rotate.py" line="289"/>
      <source>Some subelements could not be moved.</source>
      <translation>Neke pod elemente nije bilo moguće pomjeriti.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="121"/>
      <location filename="../../draftguitools/gui_ellipses.py" line="138"/>
      <source>Create Ellipse</source>
      <translation>Napravi Elipsu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="55"/>
      <source>Flip dimension</source>
      <translation>Preokreni dimenziju</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="73"/>
      <source>No active Draft Toolbar.</source>
      <translation>Nema aktivne Nacrt radne trake</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="93"/>
      <source>Construction mode</source>
      <translation>Konstrukcijski način</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="125"/>
      <source>Continue mode</source>
      <translation>Nastaviti način</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="159"/>
      <source>Toggle display mode</source>
      <translation>Prebaci način prikaza</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="68"/>
      <source>Annotation style editor</source>
      <translation>Uređivač stilova napomena</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="290"/>
      <source>Open styles file</source>
      <translation>Otvori datoteku stila</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="292"/>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="314"/>
      <source>JSON file (*.json)</source>
      <translation>JSON datoteke (*.json)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="312"/>
      <source>Save styles file</source>
      <translation>Spremi datoteku stila</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="51"/>
      <source>Heal</source>
      <translation>Izliječiti</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="134"/>
      <location filename="../../draftguitools/gui_points.py" line="147"/>
      <source>Create Point</source>
      <translation>Stvori točku</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="224"/>
      <source>Create Polygon (Part)</source>
      <translation>Stvori Poligon (Dio)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="243"/>
      <source>Create Polygon</source>
      <translation>Stvori Poligon</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="76"/>
      <source>Select an object to offset</source>
      <translation>Odaberite objekt za pomak</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="82"/>
      <source>Offset only works on one object at a time.</source>
      <translation>Pomak trenutno radi samo na jednom objektu.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="92"/>
      <source>Cannot offset this object type</source>
      <translation>Nije moguće pomaknuti ovu vrstu objekta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="123"/>
      <source>Offset of Bezier curves is currently not supported</source>
      <translation>Žao nam je, pomak Bezierove krivulje trenutno još uvijek nije podržan</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="79"/>
      <source>Select an object to rotate</source>
      <translation>Odaberite objekt za rotaciju</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="99"/>
      <source>Pick rotation center</source>
      <translation>Odaberite središte rotacije</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="193"/>
      <location filename="../../draftguitools/gui_rotate.py" line="396"/>
      <source>Base angle</source>
      <translation>Kut odredišta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="194"/>
      <location filename="../../draftguitools/gui_rotate.py" line="397"/>
      <source>The base angle you wish to start the rotation from</source>
      <translation>Osnovni kut od kojeg želite započeti rotaciju</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="199"/>
      <location filename="../../draftguitools/gui_rotate.py" line="400"/>
      <source>Pick base angle</source>
      <translation>Odaberite kut odredišta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="205"/>
      <location filename="../../draftguitools/gui_rotate.py" line="409"/>
      <source>Rotation</source>
      <translation>Rotacija</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="206"/>
      <location filename="../../draftguitools/gui_rotate.py" line="410"/>
      <source>The amount of rotation you wish to perform.
The final angle will be the base angle plus this amount.</source>
      <translation>Količina rotacije koju želite izvesti.
Konačni kut bit će osnovni kut plus ovaj iznos.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="213"/>
      <location filename="../../draftguitools/gui_rotate.py" line="418"/>
      <source>Pick rotation angle</source>
      <translation>Odaberite kut rotacije</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="109"/>
      <source>Create 2D view</source>
      <translation>Stvaranje 2D prikaza</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="77"/>
      <source>Select an object to array</source>
      <translation>Odaberite jedan objekt za nizanje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="101"/>
      <source>Array</source>
      <translation>Polje</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="64"/>
      <source>Click anywhere on a line to split it.</source>
      <translation>Kliknite bilo gdje na liniji da biste je podijelili.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="106"/>
      <source>Split line</source>
      <translation>Razdvoji liniju</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="83"/>
      <source>Task panel:</source>
      <translation>Ploča zadataka:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="187"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="208"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="201"/>
      <source>At least one element must be selected.</source>
      <translation>Barem jedna stavka mora biti označena.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="205"/>
      <source>Number of elements must be at least 1.</source>
      <translation>Broj elemenata mora biti najmanje 1.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="194"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="219"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="212"/>
      <source>Selection is not suitable for array.</source>
      <translation>Odabir nije prikladan za matricu.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="195"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="327"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="220"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="372"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="213"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="375"/>
      <source>Object:</source>
      <translation>Objekt:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="316"/>
      <source>Interval X reset:</source>
      <translation>Interval X resetirati:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="325"/>
      <source>Interval Y reset:</source>
      <translation>Interval Y resetirati:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="334"/>
      <source>Interval Z reset:</source>
      <translation>Interval Z resetirati:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="296"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="341"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="344"/>
      <source>Fuse:</source>
      <translation>Spoji:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="310"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="355"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="358"/>
      <source>Create Link array:</source>
      <translation>Stvori niz poveznica:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="376"/>
      <source>Number of X elements:</source>
      <translation>Broj X elemenata:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="378"/>
      <source>Interval X:</source>
      <translation>Interval X:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="381"/>
      <source>Number of Y elements:</source>
      <translation>Broj Y elemenata:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="383"/>
      <source>Interval Y:</source>
      <translation>Interval Y:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="386"/>
      <source>Number of Z elements:</source>
      <translation>Broj Z elemenata:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="388"/>
      <source>Interval Z:</source>
      <translation>Interval Z:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="434"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="481"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="396"/>
      <source>Aborted:</source>
      <translation>Obustavljeno:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="212"/>
      <source>Number of layers must be at least 2.</source>
      <translation>Broj slojeva mora biti najmanje 2.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="224"/>
      <source>Radial distance is zero. Resulting array may not look correct.</source>
      <translation>Radijalna udaljenost je nula. Rezultirajući niz možda neće izgledati ispravno.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="226"/>
      <source>Radial distance is negative. It is made positive to proceed.</source>
      <translation>Radijalna udaljenost je negativna. biti će pozitivno napravljeno za nastaviti.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="230"/>
      <source>Tangential distance cannot be zero.</source>
      <translation>Tangencijalna udaljenost ne može biti nula.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="233"/>
      <source>Tangential distance is negative. It is made positive to proceed.</source>
      <translation>Tangencijalna udaljenost je negativna. biti će pozitivno napravljeno za nastaviti.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="286"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="331"/>
      <source>Center reset:</source>
      <translation>Ponovo postavljanje centra:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="373"/>
      <source>Radial distance:</source>
      <translation>Radijalna udaljenost:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="374"/>
      <source>Tangential distance:</source>
      <translation>Tangencijalna udaljenost:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="375"/>
      <source>Number of circular layers:</source>
      <translation>Broj kružnih slojeva:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="376"/>
      <source>Symmetry parameter:</source>
      <translation>Parametar simetrije:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="331"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="378"/>
      <source>Center of rotation:</source>
      <translation>Centar rotacije:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="199"/>
      <source>Number of elements must be at least 2.</source>
      <translation>Broj elemenata mora biti najmanje 2.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="203"/>
      <source>The angle is above 360 degrees. It is set to this value to proceed.</source>
      <translation>Kut je iznad 360 stupnjeva. Postavljen je na ovu vrijednost, za nastavak.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="206"/>
      <source>The angle is below -360 degrees. It is set to this value to proceed.</source>
      <translation>Kut je ispod -360 stupnjeva. Postavljen je na ovu vrijednost, za nastavak.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="328"/>
      <source>Number of elements:</source>
      <translation>Broj elemenata:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="329"/>
      <source>Polar angle:</source>
      <translation>Polarni kut:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="56"/>
      <source>ShapeString</source>
      <translation>Oblik Teksta</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="70"/>
      <source>Default</source>
      <translation>Inicijalno</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="361"/>
      <source>Activate this layer</source>
      <translation>Aktiviraj ovaj sloj</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="367"/>
      <source>Select layer contents</source>
      <translation>Odaberite sadržaj sloja</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="405"/>
      <location filename="../../draftviewproviders/view_layer.py" line="421"/>
      <source>Merge layer duplicates</source>
      <translation>Spoji duplikate sloja</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="410"/>
      <location filename="../../draftviewproviders/view_layer.py" line="469"/>
      <source>Add new layer</source>
      <translation>Dodaj novi sloj</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="454"/>
      <source>Relabeling layer:</source>
      <translation>Ponovno označavanje sloja:</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="458"/>
      <source>Merging layer:</source>
      <translation>Spajnje sloja:</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_base.py" line="402"/>
      <source>Please load the Draft Workbench to enable editing this object</source>
      <translation type="unfinished">Please load the Draft Workbench to enable editing this object</translation>
    </message>
  </context>
  <context>
    <name>importOCA</name>
    <message>
      <location filename="../../importOCA.py" line="360"/>
      <source>OCA error: couldn't determine character encoding</source>
      <translation>OCA pogreška: nije moguće odrediti kodiranje znakova</translation>
    </message>
    <message>
      <location filename="../../importOCA.py" line="445"/>
      <source>OCA: found no data to export</source>
      <translation>OCA: nisu pronađeni podaci za izvoz</translation>
    </message>
    <message>
      <location filename="../../importOCA.py" line="490"/>
      <source>successfully exported</source>
      <translation>uspješno izvezeno</translation>
    </message>
  </context>
</TS>
