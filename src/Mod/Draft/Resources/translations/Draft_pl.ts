<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="1.1" language="pl" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="fillet.py" line="47"/>
      <source>The start point of this line.</source>
      <translation>Punkt początkowy tej linii.</translation>
    </message>
    <message>
      <location filename="fillet.py" line="55"/>
      <source>The end point of this line.</source>
      <translation>Punkt końcowy tej linii.</translation>
    </message>
    <message>
      <location filename="fillet.py" line="63"/>
      <source>The length of this line.</source>
      <translation>Długość tej linii.</translation>
    </message>
    <message>
      <location filename="fillet.py" line="71"/>
      <source>Radius to use to fillet the corner.</source>
      <translation>Promień użyty do zaokrąglenia narożnika.</translation>
    </message>
    <message>
      <location filename="patharray.py" line="169"/>
      <source>The base object that will be duplicated</source>
      <translation>Obiekt podstawowy, który zostanie zduplikowany</translation>
    </message>
    <message>
      <location filename="array.py" line="98"/>
      <source>The type of array to create.
- Ortho: places the copies in the direction of the global X, Y, Z axes.
- Polar: places the copies along a circular arc, up to a specified angle, and with certain orientation defined by a center and an axis.
- Circular: places the copies in concentric circular layers around the base object.</source>
      <translation>Typ szyku, który ma zostać utworzony.
- Orto: umieszcza kopie w kierunku globalnych osi X, Y, Z.
- Polarny: umieszcza kopie wzdłuż łuku kołowego, aż do określonego kąta i z określoną orientacją zdefiniowaną przez środek i oś.
- Okrągły: umieszcza kopie w koncentrycznych warstwach kołowych wokół obiektu bazowego.</translation>
    </message>
    <message>
      <location filename="array.py" line="109"/>
      <source>Specifies if the copies should be fused together if they touch each other (slower)</source>
      <translation>Określa, czy kopie powinny być połączone razem, jeśli dotykają się wzajemnie (wolniej)</translation>
    </message>
    <message>
      <location filename="array.py" line="122"/>
      <source>Number of copies in X direction</source>
      <translation>Liczba kopii w kierunku X</translation>
    </message>
    <message>
      <location filename="array.py" line="131"/>
      <source>Number of copies in Y direction</source>
      <translation>Liczba kopii w kierunku Y</translation>
    </message>
    <message>
      <location filename="array.py" line="140"/>
      <source>Number of copies in Z direction</source>
      <translation>Liczba kopii w kierunku Z</translation>
    </message>
    <message>
      <location filename="array.py" line="150"/>
      <source>Distance and orientation of intervals in X direction</source>
      <translation>Odległość i orientacja odstępów w kierunku X</translation>
    </message>
    <message>
      <location filename="array.py" line="160"/>
      <source>Distance and orientation of intervals in Y direction</source>
      <translation>Odległość i orientacja odstępów w kierunku Y</translation>
    </message>
    <message>
      <location filename="array.py" line="170"/>
      <source>Distance and orientation of intervals in Z direction</source>
      <translation>Odległość i orientacja odstępów w kierunku Z</translation>
    </message>
    <message>
      <location filename="array.py" line="185"/>
      <source>The axis direction around which the elements in a polar or a circular array will be created</source>
      <translation>Kierunek osi, wokół którego będą tworzone elementy w szyku biegunowym lub kołowym</translation>
    </message>
    <message>
      <location filename="array.py" line="196"/>
      <source>Center point for polar and circular arrays.
The 'Axis' passes through this point.</source>
      <translation>Punkt środkowy dla szyków biegunowych lub kołowych
Oś przechodzi przez ten punkt.</translation>
    </message>
    <message>
      <location filename="array.py" line="215"/>
      <source>The axis object that overrides the value of 'Axis' and 'Center', for example, a datum line.
Its placement, position and rotation, will be used when creating polar and circular arrays.
Leave this property empty to be able to set 'Axis' and 'Center' manually.</source>
      <translation>Oś obiektu, która zastępuje wartość "Oś" i "Centrum", na przykład linię odniesienia.
Jej umiejscowienie, pozycja i obroty będą używane podczas tworzenia tablic polarnych i okrągłych.
Pozostaw tę właściwość pustą, aby móc ręcznie ustawić 'Oś' i 'Centrum'.</translation>
    </message>
    <message>
      <location filename="array.py" line="228"/>
      <source>Number of copies in the polar direction</source>
      <translation>Liczba kopii w kierunku biegunowym</translation>
    </message>
    <message>
      <location filename="array.py" line="238"/>
      <source>Distance and orientation of intervals in 'Axis' direction</source>
      <translation>Odległość i orientacja przedziałów w kierunku 'osi'</translation>
    </message>
    <message>
      <location filename="array.py" line="247"/>
      <source>Angle to cover with copies</source>
      <translation type="unfinished">Angle to cover with copies</translation>
    </message>
    <message>
      <location filename="array.py" line="260"/>
      <source>Distance between circular layers</source>
      <translation>Odległość między warstwami po okręgu</translation>
    </message>
    <message>
      <location filename="array.py" line="270"/>
      <source>Distance between copies in the same circular layer</source>
      <translation>Odległość między kopiami w tej samej warstwie kołowej</translation>
    </message>
    <message>
      <location filename="array.py" line="280"/>
      <source>Number of circular layers. The 'Base' object counts as one layer.</source>
      <translation>Liczba warstw kołowych. Obiekt 'Baza' liczy się jako jedna warstwa.</translation>
    </message>
    <message>
      <location filename="array.py" line="291"/>
      <source>A parameter that determines how many symmetry planes  the circular array will have.</source>
      <translation>Parametr określający, ile płaszczyzn symetrii będzie mieć szyk kołowy.</translation>
    </message>
    <message>
      <location filename="array.py" line="309"/>
      <source>Total number of elements in the array.
This property is read-only, as the number depends on the parameters of the array.</source>
      <translation>Całkowita liczba elementów w tablicy.
Ta właściwość jest tylko do odczytu, ponieważ liczba zależy od parametrów tablicy.</translation>
    </message>
    <message>
      <location filename="patharray.py" line="208"/>
      <source>Show the individual array elements (only for Link arrays)</source>
      <translation>Pokaż poszczególne elementy tablicy (tylko dla tablic linków)</translation>
    </message>
    <message>
      <location filename="block.py" line="43"/>
      <source>The components of this block</source>
      <translation>Składniki tego bloku</translation>
    </message>
    <message>
      <location filename="wpproxy.py" line="42"/>
      <source>The placement of this object</source>
      <translation>Położenie tego obiektu</translation>
    </message>
    <message>
      <location filename="rectangle.py" line="46"/>
      <source>Length of the rectangle</source>
      <translation>Długość prostokąta</translation>
    </message>
    <message>
      <location filename="rectangle.py" line="49"/>
      <source>Height of the rectangle</source>
      <translation>Wysokość prostokąta</translation>
    </message>
    <message>
      <location filename="wire.py" line="77"/>
      <source>Radius to use to fillet the corners</source>
      <translation>Promień użyty do zaokrąglenia narożników</translation>
    </message>
    <message>
      <location filename="wire.py" line="81"/>
      <source>Size of the chamfer to give to the corners</source>
      <translation>Rozmiar fazy, którą należy nadać narożnikom</translation>
    </message>
    <message>
      <location filename="ellipse.py" line="58"/>
      <source>Create a face</source>
      <translation>Utwórz ścianę</translation>
    </message>
    <message>
      <location filename="rectangle.py" line="61"/>
      <source>Horizontal subdivisions of this rectangle</source>
      <translation>Poziome podziały tego prostokąta</translation>
    </message>
    <message>
      <location filename="rectangle.py" line="64"/>
      <source>Vertical subdivisions of this rectangle</source>
      <translation>Pionowe podziały tego prostokąta</translation>
    </message>
    <message>
      <location filename="wire.py" line="93"/>
      <source>The area of this object</source>
      <translation>Obszar tego obiektu</translation>
    </message>
    <message>
      <location filename="dimension.py" line="137"/>
      <source>The normal direction of the text of the dimension</source>
      <translation>Normalny kierunek tekstu wymiaru</translation>
    </message>
    <message>
      <location filename="dimension.py" line="150"/>
      <source>The object measured by this dimension object</source>
      <translation>Obiekt mierzony przez ten wymiar</translation>
    </message>
    <message>
      <location filename="dimension.py" line="167"/>
      <source>The object, and specific subelements of it,
that this dimension object is measuring.

There are various possibilities:
- An object, and one of its edges.
- An object, and two of its vertices.
- An arc object, and its edge.
</source>
      <translation>Obiekt i określone jego części,
są mierzone przez ten obiekt wymiaru.

Istnieją różne możliwości:
- Obiekt i jedna z jego krawędzi.
- Obiekt i dwa wierzchołki.
- Obiekt łuku i jego krawędź.
</translation>
    </message>
    <message>
      <location filename="dimension.py" line="190"/>
      <source>A point through which the dimension line, or an extrapolation of it, will pass.

- For linear dimensions, this property controls how close the dimension line
is to the measured object.
- For radial dimensions, this controls the direction of the dimension line
that displays the measured radius or diameter.
- For angular dimensions, this controls the radius of the dimension arc
that displays the measured angle.</source>
      <translation>Punkt, przez który przechodzi linia wymiaru lub jej ekstrapolacja.

- Dla wymiarów liniowych ta właściwość kontroluje jak blisko linia wymiaru
jest od mierzonego obiektu.
- W przypadku wymiarów promieniowych kontroluje to kierunek linii wymiarowej,
która wyświetla zmierzony promień lub średnicę.
- Dla wymiarów kątowych kontroluje promień łuku linii wymiarowej,
który wyświetla zmierzony kąt.</translation>
    </message>
    <message>
      <location filename="dimension.py" line="233"/>
      <source>Starting point of the dimension line.

If it is a radius dimension it will be the center of the arc.
If it is a diameter dimension it will be a point that lies on the arc.</source>
      <translation>Punkt początkowy linii wymiarowej.

Jeśli jest to wymiar promienia, będzie to środek łuku.
Jeśli jest to wymiar średnicy, będzie to punkt leżący na łuku.</translation>
    </message>
    <message>
      <location filename="dimension.py" line="247"/>
      <source>Ending point of the dimension line.

If it is a radius or diameter dimension
it will be a point that lies on the arc.</source>
      <translation>Punkt końcowy linii wymiarowej.

Jeśli jest to promień lub wymiar średnicy, będzie to punkt leżący na łuku.</translation>
    </message>
    <message>
      <location filename="dimension.py" line="259"/>
      <source>The direction of the dimension line.
If this remains '(0,0,0)', the direction will be calculated automatically.</source>
      <translation>Kierunek linii wymiarowej.
Jeśli pozostanie to '(0,0,0)', kierunek będzie obliczany automatycznie.</translation>
    </message>
    <message>
      <location filename="dimension.py" line="276"/>
      <source>The value of the measurement.

This property is read-only because the value is calculated
from the 'Start' and 'End' properties.

If the 'Linked Geometry' is an arc or circle, this 'Distance'
is the radius or diameter, depending on the 'Diameter' property.</source>
      <translation>Wartość pomiaru.

Ta właściwość jest tylko do odczytu, ponieważ wartość jest obliczana
z właściwości 'Start' i 'Koniec'.

Jeśli 'Połączona geometria' jest łukiem lub kołem, to 'Odległość'
jest promieniem lub średnicą, w zależności od właściwości 'średnica'.</translation>
    </message>
    <message>
      <location filename="dimension.py" line="287"/>
      <source>When measuring circular arcs, it determines whether to display
the radius or the diameter value</source>
      <translation>Podczas pomiaru łuków kołowych określa, czy wyświetlić
wartość promienia czy wartość średnicy</translation>
    </message>
    <message>
      <location filename="dimension.py" line="507"/>
      <source>Starting angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
      <translation>Kąt początkowy linii wymiarowej (łuk kołowy).
Łuk jest rysowany przeciwnie do ruchu wskazówek zegara.</translation>
    </message>
    <message>
      <location filename="dimension.py" line="518"/>
      <source>Ending angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
      <translation>Kąt końcowy linii wymiarowej (łuk kołowy).
Łuk jest rysowany przeciwnie do ruchu wskazówek zegara.</translation>
    </message>
    <message>
      <location filename="dimension.py" line="533"/>
      <source>The center point of the dimension line, which is a circular arc.

This is normally the point where two line segments, or their extensions
intersect, resulting in the measured 'Angle' between them.</source>
      <translation>Punkt środkowy linii wymiarowej, która jest łukiem kołowym.

Jest to zazwyczaj punkt, w którym dwa odcinki linii lub ich rozszerzenia
przecinają się, co skutkuje zmierzeniem „Kąt” między nimi.</translation>
    </message>
    <message>
      <location filename="dimension.py" line="547"/>
      <source>The value of the measurement.

This property is read-only because the value is calculated from
the 'First Angle' and 'Last Angle' properties.</source>
      <translation>Wartość pomiaru.

Ta właściwość jest tylko do odczytu, ponieważ wartość jest obliczana z
właściwości „Pierwszy kąt” i „ostatni kąt”.</translation>
    </message>
    <message>
      <location filename="text.py" line="54"/>
      <source>The placement of the base point of the first line</source>
      <translation>Położenie punktu bazowego pierwszej linii</translation>
    </message>
    <message>
      <location filename="text.py" line="66"/>
      <source>The text displayed by this object.
It is a list of strings; each element in the list will be displayed in its own line.</source>
      <translation>Tekst wyświetlany przez ten obiekt.
Jest to lista ciągów; każdy element na liście będzie wyświetlany w osobnej linii.</translation>
    </message>
    <message>
      <location filename="circle.py" line="45"/>
      <source>Start angle of the arc</source>
      <translation>Kąt początkowy łuku</translation>
    </message>
    <message>
      <location filename="circle.py" line="50"/>
      <source>End angle of the arc (for a full circle, 
                give it same value as First Angle)</source>
      <translation>Końcowy kąt łuku (dla pełnego okręgu, daj taką samą wartość jak dla pierwszego kąta)</translation>
    </message>
    <message>
      <location filename="circle.py" line="54"/>
      <source>Radius of the circle</source>
      <translation>Promień okręgu</translation>
    </message>
    <message>
      <location filename="polygon.py" line="48"/>
      <source>Number of faces</source>
      <translation>Liczba ścian</translation>
    </message>
    <message>
      <location filename="polygon.py" line="52"/>
      <source>Radius of the control circle</source>
      <translation>Promień okręgu kontrolnego</translation>
    </message>
    <message>
      <location filename="polygon.py" line="56"/>
      <source>How the polygon must be drawn from the control circle</source>
      <translation>Sposób narysowania wielokąta z okręgu kontrolnego</translation>
    </message>
    <message>
      <location filename="point.py" line="45"/>
      <source>X Location</source>
      <translation>Pozycja X</translation>
    </message>
    <message>
      <location filename="point.py" line="48"/>
      <source>Y Location</source>
      <translation>Pozycja Y</translation>
    </message>
    <message>
      <location filename="point.py" line="51"/>
      <source>Z Location</source>
      <translation>Pozycja Z</translation>
    </message>
    <message>
      <location filename="layer.py" line="59"/>
      <source>The objects that are part of this layer</source>
      <translation>Obiekty będące częścią tej warstwy</translation>
    </message>
    <message>
      <location filename="label.py" line="65"/>
      <source>The position of the tip of the leader line.
This point can be decorated with an arrow or another symbol.</source>
      <translation type="unfinished">The position of the tip of the leader line.
This point can be decorated with an arrow or another symbol.</translation>
    </message>
    <message>
      <location filename="label.py" line="79"/>
      <source>Object, and optionally subelement, whose properties will be displayed
as 'Text', depending on 'Label Type'.

'Target' won't be used if 'Label Type' is set to 'Custom'.</source>
      <translation type="unfinished">Object, and optionally subelement, whose properties will be displayed
as 'Text', depending on 'Label Type'.

'Target' won't be used if 'Label Type' is set to 'Custom'.</translation>
    </message>
    <message>
      <location filename="label.py" line="109"/>
      <source>The list of points defining the leader line; normally a list of three points.

The first point should be the position of the text, that is, the 'Placement',
and the last point should be the tip of the line, that is, the 'Target Point'.
The middle point is calculated automatically depending on the chosen
'Straight Direction' and the 'Straight Distance' value and sign.

If 'Straight Direction' is set to 'Custom', the 'Points' property
can be set as a list of arbitrary points.</source>
      <translation type="unfinished">The list of points defining the leader line; normally a list of three points.

The first point should be the position of the text, that is, the 'Placement',
and the last point should be the tip of the line, that is, the 'Target Point'.
The middle point is calculated automatically depending on the chosen
'Straight Direction' and the 'Straight Distance' value and sign.

If 'Straight Direction' is set to 'Custom', the 'Points' property
can be set as a list of arbitrary points.</translation>
    </message>
    <message>
      <location filename="label.py" line="124"/>
      <source>The direction of the straight segment of the leader line.

If 'Custom' is chosen, the points of the leader can be specified by
assigning a custom list to the 'Points' attribute.</source>
      <translation type="unfinished">The direction of the straight segment of the leader line.

If 'Custom' is chosen, the points of the leader can be specified by
assigning a custom list to the 'Points' attribute.</translation>
    </message>
    <message>
      <location filename="label.py" line="142"/>
      <source>The length of the straight segment of the leader line.

This is an oriented distance; if it is negative, the line will be drawn
to the left or below the 'Text', otherwise to the right or above it,
depending on the value of 'Straight Direction'.</source>
      <translation type="unfinished">The length of the straight segment of the leader line.

This is an oriented distance; if it is negative, the line will be drawn
to the left or below the 'Text', otherwise to the right or above it,
depending on the value of 'Straight Direction'.</translation>
    </message>
    <message>
      <location filename="label.py" line="156"/>
      <source>The placement of the 'Text' element in 3D space</source>
      <translation>Umieszczenie elementu "Tekst" w przestrzeni 3D</translation>
    </message>
    <message>
      <location filename="label.py" line="166"/>
      <source>The text to display when 'Label Type' is set to 'Custom'</source>
      <translation>Tekst do wyświetlenia, gdy "Typ etykiety" jest ustawiony na "Własny"</translation>
    </message>
    <message>
      <location filename="label.py" line="181"/>
      <source>The text displayed by this label.

This property is read-only, as the final text depends on 'Label Type',
and the object defined in 'Target'.
The 'Custom Text' is displayed only if 'Label Type' is set to 'Custom'.</source>
      <translation>Tekst wyświetlany przez tę etykietę.

Ta właściwość jest tylko do odczytu, ponieważ ostateczny tekst zależy od 'Etykiety',
i obiektu zdefiniowanego w 'Target'.
Tekst niestandardowy jest wyświetlany tylko wtedy, gdy "Typ etykiety" jest ustawiony na "Własny".</translation>
    </message>
    <message>
      <location filename="label.py" line="216"/>
      <source>The type of information displayed by this label.

If 'Custom' is chosen, the contents of 'Custom Text' will be used.
For other types, the string will be calculated automatically from the object defined in 'Target'.
'Tag' and 'Material' only work for objects that have these properties, like Arch objects.

For 'Position', 'Length', and 'Area' these properties will be extracted from the main object in 'Target',
or from the subelement 'VertexN', 'EdgeN', or 'FaceN', respectively, if it is specified.</source>
      <translation>Typ informacji wyświetlanych przez tę etykietę.

Jeśli wybrano opcję "Własny", zostanie użyta zawartość "Niestandardowego tekstu".
Dla innych typów, ciąg będzie obliczany automatycznie na podstawie obiektu zdefiniowanego w 'Target'.
'Tag' i 'Materiał' działają tylko dla obiektów, które mają te właściwości, jak obiekty Arch.

Dla 'Pozycji', 'Długości' i 'Powierzchni' te właściwości zostaną wydobyte z głównego obiektu w 'Target',
lub z subelement 'VertexN', 'EdgeN' lub 'FaceN', jeśli jest określony.</translation>
    </message>
    <message>
      <location filename="shapestring.py" line="47"/>
      <source>Text string</source>
      <translation>Ciąg znaków tekstowych</translation>
    </message>
    <message>
      <location filename="shapestring.py" line="50"/>
      <source>Font file name</source>
      <translation>Nazwa pliku czcionki</translation>
    </message>
    <message>
      <location filename="shapestring.py" line="53"/>
      <source>Height of text</source>
      <translation>Wysokość tekstu</translation>
    </message>
    <message>
      <location filename="shapestring.py" line="56"/>
      <source>Inter-character spacing</source>
      <translation>Odstęp pomiędzy znakami</translation>
    </message>
    <message>
      <location filename="draftlink.py" line="104"/>
      <source>Show the individual array elements</source>
      <translation>Pokaż poszczególne elementy tablicy</translation>
    </message>
    <message>
      <location filename="pointarray.py" line="79"/>
      <source>Base object that will be duplicated</source>
      <translation>Obiekt bazowy, który musi zostać zduplikowany</translation>
    </message>
    <message>
      <location filename="pointarray.py" line="87"/>
      <source>Object containing points used to distribute the base object, for example, a sketch or a Part compound.
The sketch or compound must contain at least one explicit point or vertex object.</source>
      <translation>Obiekt zawierający punkty używane do rozprowadzania podstawowego obiektu, na przykład szkic lub składnik części.
Szkic lub związek musi zawierać co najmniej jeden wyraźny punkt lub obiekt wierzchołkowy.</translation>
    </message>
    <message>
      <location filename="pointarray.py" line="95"/>
      <source>Total number of elements in the array.
This property is read-only, as the number depends on the points contained within 'Point Object'.</source>
      <translation>Całkowita liczba elementów w tablicy.
Ta właściwość jest tylko do odczytu, ponieważ liczba zależy od parametrów tablicy.</translation>
    </message>
    <message>
      <location filename="pointarray.py" line="139"/>
      <source>Additional placement, shift and rotation, that will be applied to each copy</source>
      <translation>Dodatkowe rozmieszczenie, przesunięcie i obrót, które zostaną zastosowane do każdej kopii</translation>
    </message>
    <message>
      <location filename="bspline.py" line="46"/>
      <source>The points of the B-spline</source>
      <translation>Punkty B-spline</translation>
    </message>
    <message>
      <location filename="bspline.py" line="50"/>
      <source>If the B-spline is closed or not</source>
      <translation>Jeśli B-spline jest zamknięty lub nie</translation>
    </message>
    <message>
      <location filename="bspline.py" line="54"/>
      <source>Create a face if this spline is closed</source>
      <translation>Utwórz ścianę jeśli ta krzywa składana jest zamknięta</translation>
    </message>
    <message>
      <location filename="bspline.py" line="67"/>
      <source>Parameterization factor</source>
      <translation>Współczynnik parametryzacji</translation>
    </message>
    <message>
      <location filename="shape2dview.py" line="49"/>
      <source>The base object this 2D view must represent</source>
      <translation type="unfinished">The base object this 2D view must represent</translation>
    </message>
    <message>
      <location filename="shape2dview.py" line="54"/>
      <source>The projection vector of this object</source>
      <translation>Wektor rzutowania tego obiektu</translation>
    </message>
    <message>
      <location filename="shape2dview.py" line="59"/>
      <source>The way the viewed object must be projected</source>
      <translation>Sposób rzutowania oglądanego obiektu</translation>
    </message>
    <message>
      <location filename="shape2dview.py" line="64"/>
      <source>The indices of the faces to be projected in Individual Faces mode</source>
      <translation>Wskaźniki powierzchni będą wyświetlane w trybie indywidualnych powierzchni</translation>
    </message>
    <message>
      <location filename="shape2dview.py" line="69"/>
      <source>Show hidden lines</source>
      <translation>Pokaż ukryte linie</translation>
    </message>
    <message>
      <location filename="shape2dview.py" line="74"/>
      <source>Fuse wall and structure objects of same type and material</source>
      <translation>Powiąż ścianę i obiekty struktury tego samego typu i materiału</translation>
    </message>
    <message>
      <location filename="shape2dview.py" line="79"/>
      <source>Tessellate Ellipses and B-splines into line segments</source>
      <translation>Elipsy Mozaikowe i Krzywe B-sklejone w segmentach liniowych</translation>
    </message>
    <message>
      <location filename="shape2dview.py" line="85"/>
      <source>For Cutlines and Cutfaces modes, 
                this leaves the faces at the cut location</source>
      <translation type="unfinished">For Cutlines and Cutfaces modes, 
                this leaves the faces at the cut location</translation>
    </message>
    <message>
      <location filename="shape2dview.py" line="91"/>
      <source>Length of line segments if tessellating Ellipses or B-splines 
                into line segments</source>
      <translation>Długość segmentów liniowych, jeśli mozaikowe elipsy lub krzywe B-sklejone są
                w segmentach liniowych</translation>
    </message>
    <message>
      <location filename="shape2dview.py" line="97"/>
      <source>If this is True, this object will be recomputed only if it is 
                visible</source>
      <translation>Jeśli to jest Prawda, obiekt ten zostanie przeliczony tylko wtedy, gdy będzie widoczny</translation>
    </message>
    <message>
      <location filename="ellipse.py" line="45"/>
      <source>Start angle of the elliptical arc</source>
      <translation>Kąt początkowy łuku eliptycznego</translation>
    </message>
    <message>
      <location filename="ellipse.py" line="49"/>
      <source>End angle of the elliptical arc 

                (for a full circle, give it same value as First Angle)</source>
      <translation>Kąt końcowy łuku eliptycznego 

                (dla pełnego okręgu nadaj mu tę samą wartość co pierwszemu kątowi)</translation>
    </message>
    <message>
      <location filename="ellipse.py" line="52"/>
      <source>Minor radius of the ellipse</source>
      <translation>Mniejszy promień elipsy</translation>
    </message>
    <message>
      <location filename="ellipse.py" line="55"/>
      <source>Major radius of the ellipse</source>
      <translation>Większy promień elipsy</translation>
    </message>
    <message>
      <location filename="ellipse.py" line="61"/>
      <source>Area of this object</source>
      <translation>Obszar tego obiektu</translation>
    </message>
    <message>
      <location filename="bezcurve.py" line="46"/>
      <source>The points of the Bezier curve</source>
      <translation>Punkty krzywej Beziera</translation>
    </message>
    <message>
      <location filename="bezcurve.py" line="50"/>
      <source>The degree of the Bezier function</source>
      <translation>Stopień funkcji Beziera</translation>
    </message>
    <message>
      <location filename="bezcurve.py" line="54"/>
      <source>Continuity</source>
      <translation>Ciągłość</translation>
    </message>
    <message>
      <location filename="bezcurve.py" line="58"/>
      <source>If the Bezier curve should be closed or not</source>
      <translation>Czy krzywa Beziera ma być zamknięta czy nie</translation>
    </message>
    <message>
      <location filename="bezcurve.py" line="62"/>
      <source>Create a face if this curve is closed</source>
      <translation>Utwórz ścianę jeśli ta krzywa jest zamknięta</translation>
    </message>
    <message>
      <location filename="bezcurve.py" line="66"/>
      <source>The length of this object</source>
      <translation>Długość tego obiektu</translation>
    </message>
    <message>
      <location filename="patharray.py" line="177"/>
      <source>The object along which the copies will be distributed. It must contain 'Edges'.</source>
      <translation>Obiekt, wzdłuż którego zostaną rozmieszczone kopie. Musi on zawierać "Krawędzie".</translation>
    </message>
    <message>
      <location filename="patharray.py" line="192"/>
      <source>List of connected edges in the 'Path Object'.
If these are present, the copies will be created along these subelements only.
Leave this property empty to create copies along the entire 'Path Object'.</source>
      <translation>Lista połączonych krawędzi w 'Obiekcie ścieżki'.
Jeśli są one obecne, kopie będą tworzone tylko wzdłuż tych podelementów.
Pozostawienie pustej właściwości spowoduje tworzenie kopii wzdłuż całego 'obiektu ścieżki'.</translation>
    </message>
    <message>
      <location filename="patharray.py" line="200"/>
      <source>Number of copies to create</source>
      <translation>Liczba kopii do utworzenia</translation>
    </message>
    <message>
      <location filename="patharray.py" line="219"/>
      <source>Additional translation that will be applied to each copy.
This is useful to adjust for the difference between shape centre and shape reference point.</source>
      <translation>Dodatkowe przesunięcie, które zostanie zastosowane do każdej kopii.
Jest to użyteczne, aby dostosować się do różnicy między środkiem kształtu a punktem odniesienia kształtu.</translation>
    </message>
    <message>
      <location filename="patharray.py" line="227"/>
      <source>Alignment vector for 'Tangent' mode</source>
      <translation>Wektor wyrównania dla trybu 'Tangent'</translation>
    </message>
    <message>
      <location filename="patharray.py" line="235"/>
      <source>Force use of 'Vertical Vector' as local Z direction when using 'Original' or 'Tangent' alignment mode</source>
      <translation>Wymuś użycie „Pionowego wektora” jako lokalnego kierunku Z podczas używania trybu wyrównania „Oryginał” lub „Tangent”</translation>
    </message>
    <message>
      <location filename="patharray.py" line="243"/>
      <source>Direction of the local Z axis when 'Force Vertical' is true</source>
      <translation>Kierunek lokalnej osi Z, gdy 'Wymuszenie pionowe' jest prawdą</translation>
    </message>
    <message>
      <location filename="patharray.py" line="251"/>
      <source>Method to orient the copies along the path.
- Original: X is curve tangent, Y is normal, and Z is the cross product.
- Frenet: aligns the object following the local coordinate system along the path.
- Tangent: similar to 'Original' but the local X axis is pre-aligned to 'Tangent Vector'.

To get better results with 'Original' or 'Tangent' you may have to set 'Force Vertical' to true.</source>
      <translation>Metoda kierowania duplikatów wzdłuż ścieżki.
- Oryginał: X to styczna do krzywej, Y to normalna, a Z to ich iloczyn wektorowy.
- Frenet: wyrównuje obiekt zgodnie z lokalnym układem współrzędnych wzdłuż ścieżki.
- Styczny: podobny do „Oryginalnego”, z tą różnicą, że lokalna oś X jest wstępnie wyrównana do „Stycznego wektora”.
Aby uzyskać lepsze wyniki w trybach „Oryginal” i „Tangent”, najprawdopodobniej trzeba będzie włączyć opcję „Wymuszenie pionowe”.</translation>
    </message>
    <message>
      <location filename="patharray.py" line="262"/>
      <source>Orient the copies along the path depending on the 'Align Mode'.
Otherwise the copies will have the same orientation as the original Base object.</source>
      <translation>Prowadź duplikaty wzdłuż ścieżki zgodnie z „Trybem wyrównania”.
W przeciwnym razie duplikaty będą miały taką samą orientację jak oryginalna linia bazowa.</translation>
    </message>
    <message>
      <location filename="drawingview.py" line="66"/>
      <source>The linked object</source>
      <translation>Obiekt połączony</translation>
    </message>
    <message>
      <location filename="drawingview.py" line="73"/>
      <source>Projection direction</source>
      <translation>Kierunek projekcji</translation>
    </message>
    <message>
      <location filename="drawingview.py" line="80"/>
      <source>The width of the lines inside this object</source>
      <translation>Szerokość linii wewnątrz tego obiektu</translation>
    </message>
    <message>
      <location filename="drawingview.py" line="88"/>
      <source>The size of the texts inside this object</source>
      <translation>Rozmiar tekstów wewnątrz tego obiektu</translation>
    </message>
    <message>
      <location filename="drawingview.py" line="96"/>
      <source>The spacing between lines of text</source>
      <translation>Odstęp między wierszami tekstu</translation>
    </message>
    <message>
      <location filename="drawingview.py" line="103"/>
      <source>The color of the projected objects</source>
      <translation>Kolor rzutowanych obiektów</translation>
    </message>
    <message>
      <location filename="drawingview.py" line="110"/>
      <source>Shape Fill Style</source>
      <translation>Styl wypełnienia kształtu</translation>
    </message>
    <message>
      <location filename="drawingview.py" line="118"/>
      <source>Line Style</source>
      <translation>Styl linii</translation>
    </message>
    <message>
      <location filename="drawingview.py" line="127"/>
      <source>If checked, source objects are displayed regardless of being visible in the 3D model</source>
      <translation>Jeśli jest zaznaczone, obiekty źródłowe są wyświetlane niezależnie od tego, czy są widoczne w modelu 3D</translation>
    </message>
    <message>
      <location filename="facebinder.py" line="43"/>
      <source>Linked faces</source>
      <translation>Powiązane ściany</translation>
    </message>
    <message>
      <location filename="facebinder.py" line="46"/>
      <source>Specifies if splitter lines must be removed</source>
      <translation>Określa, czy należy usunąć linie podziału</translation>
    </message>
    <message>
      <location filename="facebinder.py" line="49"/>
      <source>An optional extrusion value to be applied to all faces</source>
      <translation>Opcjonalna wartość wyciągnięcia, która zostanie zastosowana do wszystkich ścian</translation>
    </message>
    <message>
      <location filename="facebinder.py" line="52"/>
      <source>An optional offset value to be applied to all faces</source>
      <translation>Opcjonalna wartość przesunięcia, która zostanie zastosowana do wszystkich ścian</translation>
    </message>
    <message>
      <location filename="facebinder.py" line="55"/>
      <source>This specifies if the shapes sew</source>
      <translation>Oznacza to, że kształty są zszywane razem</translation>
    </message>
    <message>
      <location filename="facebinder.py" line="58"/>
      <source>The area of the faces of this Facebinder</source>
      <translation>Pole powierzchni tego spoiwa powierzchniowego</translation>
    </message>
    <message>
      <location filename="clone.py" line="46"/>
      <source>The objects included in this clone</source>
      <translation>Obiekty są zawarte w tej kopii</translation>
    </message>
    <message>
      <location filename="clone.py" line="51"/>
      <source>The scale factor of this clone</source>
      <translation>Współczynnik skali tej kopii</translation>
    </message>
    <message>
      <location filename="clone.py" line="57"/>
      <source>If Clones includes several objects,
set True for fusion or False for compound</source>
      <translation>Jeśli Klony zawierają kilka obiektów,
ustaw True dla połączenia lub False dla kombinacji</translation>
    </message>
    <message>
      <location filename="view_draft_annotation.py" line="82"/>
      <source>General scaling factor that affects the annotation consistently
because it scales the text, and the line decorations, if any,
in the same proportion.</source>
      <translation type="unfinished">General scaling factor that affects the annotation consistently
because it scales the text, and the line decorations, if any,
in the same proportion.</translation>
    </message>
    <message>
      <location filename="view_draft_annotation.py" line="100"/>
      <source>Annotation style to apply to this object.
When using a saved style some of the view properties will become read-only;
they will only be editable by changing the style through the 'Annotation style editor' tool.</source>
      <translation type="unfinished">Annotation style to apply to this object.
When using a saved style some of the view properties will become read-only;
they will only be editable by changing the style through the 'Annotation style editor' tool.</translation>
    </message>
    <message>
      <location filename="wire.py" line="49"/>
      <source>The vertices of the wire</source>
      <translation>Wierzchołki linii łamanej</translation>
    </message>
    <message>
      <location filename="wire.py" line="53"/>
      <source>If the wire is closed or not</source>
      <translation>Jeśli linia łamana jest zamknięta lub nie</translation>
    </message>
    <message>
      <location filename="wire.py" line="57"/>
      <source>The base object is the wire, it's formed from 2 objects</source>
      <translation>Obiektem podstawowym jest linia łamana, utworzona z 2 obiektów</translation>
    </message>
    <message>
      <location filename="wire.py" line="61"/>
      <source>The tool object is the wire, it's formed from 2 objects</source>
      <translation>Obiektem podstawowym jest linia łamana, utworzona z 2 obiektów</translation>
    </message>
    <message>
      <location filename="wire.py" line="65"/>
      <source>The start point of this line</source>
      <translation>Punkt początkowy tej linii</translation>
    </message>
    <message>
      <location filename="wire.py" line="69"/>
      <source>The end point of this line</source>
      <translation>Punkt końcowy tej linii</translation>
    </message>
    <message>
      <location filename="wire.py" line="73"/>
      <source>The length of this line</source>
      <translation>Długość tej linii</translation>
    </message>
    <message>
      <location filename="wire.py" line="85"/>
      <source>Create a face if this object is closed</source>
      <translation>Utwórz ścianę jeśli ten obiekt jest zamknięty</translation>
    </message>
    <message>
      <location filename="wire.py" line="89"/>
      <source>The number of subdivisions of each edge</source>
      <translation>Liczba podziałów każdej krawędzi</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="137"/>
      <source>Font name</source>
      <translation>Nazwa czcionki</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="146"/>
      <source>Font size</source>
      <translation>Wielkość czcionki</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="155"/>
      <source>Spacing between text and dimension line</source>
      <translation>Odstęp między tekstem a linią wymiarową</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="164"/>
      <source>Rotate the dimension text 180 degrees</source>
      <translation>Obróć tekst wymiarowy o 180 stopni</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="174"/>
      <source>Text Position.
Leave '(0,0,0)' for automatic position</source>
      <translation>Położenie tekstu.
Pozostaw '(0,0,0)' dla pozycji automatycznej</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="185"/>
      <source>Text override.
Write '$dim' so that it is replaced by the dimension length.</source>
      <translation>Nadpisanie tekstu.
Napisz '$dim' aby został zastąpiony długością wymiaru.</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="196"/>
      <source>The number of decimals to show</source>
      <translation>Liczba miejsc po przecinku do pokazania</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="205"/>
      <source>Show the unit suffix</source>
      <translation>Pokaż symbol jednostki</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="215"/>
      <source>A unit to express the measurement.
Leave blank for system default</source>
      <translation>Jednostka do wyrażenia pomiaru.
Pozostaw puste aby użyć wartości domyślnej</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="229"/>
      <source>Arrow size</source>
      <translation>Rozmiar strzałki</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="238"/>
      <source>Arrow type</source>
      <translation>Styl strzałki</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="248"/>
      <source>Rotate the dimension arrows 180 degrees</source>
      <translation>Obróć strzałki wymiarowe o 180 stopni</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="259"/>
      <source>The distance the dimension line is extended
past the extension lines</source>
      <translation>Odległość na jaką linia wymiarowa jest przedłużona poza linie przedłużenia</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="268"/>
      <source>Length of the extension lines</source>
      <translation>Długość linii przedłużenia (pomocniczych)</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="278"/>
      <source>Length of the extension line
beyond the dimension line</source>
      <translation>Długość linii pomocniczej powyżej linii wymiaru</translation>
    </message>
    <message>
      <location filename="view_dimension.py" line="287"/>
      <source>Shows the dimension line and arrows</source>
      <translation>Pokazuj linie wymiarowe i strzałki</translation>
    </message>
    <message>
      <location filename="view_layer.py" line="77"/>
      <source>If it is true, the objects contained within this layer will adopt the line color of the layer</source>
      <translation>Jeśli to prawda, obiekty zawarte w tej warstwie przyjmą kolor linii warstwy</translation>
    </message>
    <message>
      <location filename="view_layer.py" line="88"/>
      <source>If it is true, the print color will be used when objects in this layer are placed on a TechDraw page</source>
      <translation>Jeśli to prawda, kolor wydruku zostanie użyty, gdy obiekty w tej warstwie zostaną umieszczone na stronie TechDraw</translation>
    </message>
    <message>
      <location filename="view_layer.py" line="102"/>
      <source>The line color of the objects contained within this layer</source>
      <translation>Kolor linii obiektów znajdujących się w tej warstwie</translation>
    </message>
    <message>
      <location filename="view_layer.py" line="116"/>
      <source>The shape color of the objects contained within this layer</source>
      <translation>Kolor kształtu obiektów znajdujących się w tej warstwie</translation>
    </message>
    <message>
      <location filename="view_layer.py" line="130"/>
      <source>The line width of the objects contained within this layer</source>
      <translation>Szerokość linii obiektów znajdujących się w tej warstwie</translation>
    </message>
    <message>
      <location filename="view_layer.py" line="142"/>
      <source>The draw style of the objects contained within this layer</source>
      <translation>Styl rysowania obiektów zawartych w tej warstwie</translation>
    </message>
    <message>
      <location filename="view_layer.py" line="153"/>
      <source>The transparency of the objects contained within this layer</source>
      <translation>Przezroczystość obiektów zawartych w tej warstwie</translation>
    </message>
    <message>
      <location filename="view_layer.py" line="164"/>
      <source>The line color of the objects contained within this layer, when used on a TechDraw page</source>
      <translation>Kolor linii obiektów znajdujących się w tej warstwie, gdy są używane na stronie TechDraw</translation>
    </message>
    <message>
      <location filename="view_label.py" line="177"/>
      <source>Line width</source>
      <translation>Szerokość linii</translation>
    </message>
    <message>
      <location filename="view_label.py" line="186"/>
      <source>Line color</source>
      <translation>Kolor linii</translation>
    </message>
    <message>
      <location filename="view_label.py" line="74"/>
      <source>The size of the text</source>
      <translation>Rozmiar tekstu</translation>
    </message>
    <message>
      <location filename="view_label.py" line="83"/>
      <source>The font of the text</source>
      <translation>Czcionka tekstu</translation>
    </message>
    <message>
      <location filename="view_label.py" line="119"/>
      <source>The vertical alignment of the text</source>
      <translation>Wyrównanie tekstu w pionie</translation>
    </message>
    <message>
      <location filename="view_label.py" line="102"/>
      <source>Text color</source>
      <translation>Kolor tekstu</translation>
    </message>
    <message>
      <location filename="view_label.py" line="128"/>
      <source>Line spacing (relative to font size)</source>
      <translation>Odstęp między wierszami (w stosunku do rozmiaru czcionki)</translation>
    </message>
    <message>
      <location filename="view_label.py" line="111"/>
      <source>The maximum number of characters on each line of the text box</source>
      <translation>Maksymalna liczba znaków w każdym wierszu pola tekstowego</translation>
    </message>
    <message>
      <location filename="view_label.py" line="139"/>
      <source>The size of the arrow</source>
      <translation>Rozmiar strzałki</translation>
    </message>
    <message>
      <location filename="view_label.py" line="148"/>
      <source>The type of arrow of this label</source>
      <translation>Typ strzałki tej etykiety</translation>
    </message>
    <message>
      <location filename="view_label.py" line="159"/>
      <source>The type of frame around the text of this object</source>
      <translation>Typ ramki wokół tekstu tego obiektu</translation>
    </message>
    <message>
      <location filename="view_label.py" line="168"/>
      <source>Display a leader line or not</source>
      <translation>Wyświetl lub ukryj linie odniesienia</translation>
    </message>
  </context>
  <context>
    <name>Dialog</name>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="14"/>
      <source>Annotation Styles Editor</source>
      <translation>Edytor stylów adnotacji</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="20"/>
      <source>Style name</source>
      <translation>Nazwa stylu</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="50"/>
      <source>Add new...</source>
      <translation>Dodaj nowy...</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="79"/>
      <source>Renames the selected style</source>
      <translation>Zmienia nazwę zaznaczonego stylu</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="82"/>
      <source>Rename</source>
      <translation>Zmień nazwę</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="110"/>
      <source>Deletes the selected style</source>
      <translation>Usuwa wybrany styl</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="113"/>
      <source>Delete</source>
      <translation>Usuń</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="164"/>
      <source>Text</source>
      <translation>Tekst</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="173"/>
      <source>Font size</source>
      <translation>Wielkość czcionki</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="183"/>
      <source>Line spacing</source>
      <translation>Odstępy między wierszami</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="193"/>
      <source>Font name</source>
      <translation>Nazwa czcionki</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="212"/>
      <source>The font to use for texts and dimensions</source>
      <translation>Czcionka używana dla tekstów i wymiarów</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="239"/>
      <source>Units</source>
      <translation>Jednostki</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="248"/>
      <source>Scale multiplier</source>
      <translation>Mnożnik skali</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="258"/>
      <source>Decimals</source>
      <translation>Dziesiętne</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="268"/>
      <source>Unit override</source>
      <translation>Zastąpienie jednostki</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="278"/>
      <source>Show unit</source>
      <translation>Wyświetlaj jednostki</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="331"/>
      <source>Line and arrows</source>
      <translation>Linia i strzałki</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="340"/>
      <source>Line width</source>
      <translation>Szerokość linii</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="350"/>
      <source>Extension overshoot</source>
      <translation>Przedłużenie linii pomocniczej</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="360"/>
      <source>Arrow size</source>
      <translation>Rozmiar strzałki</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="370"/>
      <source>Show lines</source>
      <translation>Wyświetlaj linie</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="380"/>
      <source>Dimension overshoot</source>
      <translation>Przekroczenie wymiaru</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="390"/>
      <source>Extension lines</source>
      <translation>Linie pomocnicze</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="400"/>
      <source>Arrow type</source>
      <translation>Styl strzałki</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="410"/>
      <source>Line / text color</source>
      <translation>Kolor linii/tekstu</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="433"/>
      <source>The width of the dimension lines</source>
      <translation>Szerokość linii wymiarowych</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="436"/>
      <source>px</source>
      <translation>px</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="446"/>
      <source>The color of dimension lines, arrows and texts</source>
      <translation>Kolor linii wymiarowych, grotów i opisów</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="476"/>
      <source>Dot</source>
      <translation>Kropka</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="486"/>
      <source>Arrow</source>
      <translation>Strzałka</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="491"/>
      <source>Tick</source>
      <translation>Odhacz</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="38"/>
      <source>The name of your style. Existing style names can be edited.</source>
      <translation>Twoja nazwa stylu. Nazwy stylu mogą być edytowane.</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="219"/>
      <source>Font size in the system units</source>
      <translation>Rozmiar czcionki</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="180"/>
      <source>Line spacing in system units</source>
      <translation>Odstępy między wierszami</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="285"/>
      <source>A multiplier factor that affects the size of texts and markers</source>
      <translation>Mnożnik mający wpływ na rozmiar tekstów i znaczników</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="305"/>
      <source>The number of decimals to show for dimension values</source>
      <translation>Liczba miejsc po przecinku dla wartości wymiarów</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="298"/>
      <source>Specify a valid length unit like mm, m, in, ft, to force displaying the dimension value in this unit</source>
      <translation>Określ poprawną jednostkę długości, taką jak: mm, m, cal, stopa, aby wyświetlić wartość wymiaru w tej jednostce</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="315"/>
      <source>If it is checked it will show the unit next to the dimension value</source>
      <translation>Jeśli jest zaznaczone, to pokaże jednostkę obok wartości wymiaru</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="534"/>
      <source>The distance that the extension lines are additionally extended beyond the dimension line</source>
      <translation>Odległość, którą linie przedłużenia są dodatkowo rozszerzone poza linię wymiarową</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="504"/>
      <source>The size of the dimension arrows or markers in system units</source>
      <translation>Rozmiar strzałek lub znaczników wymiaru w jednostkach systemowych</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="417"/>
      <source>If it is checked it will display the dimension line</source>
      <translation>Jeśli jest zaznaczone, wyświetli linię wymiaru</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="514"/>
      <source>The distance that the dimension line is additionally extended</source>
      <translation>Odległość, na jaką linia wymiarowa jest dodatkowo wydłużona</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="524"/>
      <source>The length of the extension lines</source>
      <translation>Długość linii przedłużenia</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="472"/>
      <source>The type of arrows or markers to use at the end of dimension lines</source>
      <translation>Typ strzałek lub znaczników do użycia na końcu linii wymiaru</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="481"/>
      <source>Circle</source>
      <translation>Okrąg</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="496"/>
      <source>Tick-2</source>
      <translation>Odhacz-2</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="120"/>
      <source>Import styles from json file</source>
      <translation>Importuj style z pliku typu json</translation>
    </message>
    <message>
      <location filename="dialog_AnnotationStyleEditor.ui" line="130"/>
      <source>Export styles to json file</source>
      <translation>Eksportuj style do pliku typu json</translation>
    </message>
  </context>
  <context>
    <name>Draft</name>
    <message>
      <location filename="importDXF.py" line="129"/>
      <source>Download of dxf libraries failed.
Please install the dxf Library addon manually
from menu Tools -&gt; Addon Manager</source>
      <translation>Pobieranie biblioteki dxf nie powiodło się.
Proszę zainstalować dodatek bibliotek dxf ręcznie z narzędzi menu -&gt; menedżer dodadków</translation>
    </message>
    <message>
      <location filename="init_tools.py" line="145"/>
      <source>Draft creation tools</source>
      <translation>Narzędzia kreślarskie</translation>
    </message>
    <message>
      <location filename="init_tools.py" line="148"/>
      <source>Draft annotation tools</source>
      <translation>Narzędzia opisów kreślarskich</translation>
    </message>
    <message>
      <location filename="init_tools.py" line="151"/>
      <source>Draft modification tools</source>
      <translation>Narzędzia do modyfikacji rysunku</translation>
    </message>
    <message>
      <location filename="InitGui.py" line="107"/>
      <source>Draft utility tools</source>
      <translation>Przybory do rysowania</translation>
    </message>
    <message>
      <location filename="init_tools.py" line="166"/>
      <source>&amp;Drafting</source>
      <translation>&amp;Kreślenie</translation>
    </message>
    <message>
      <location filename="init_tools.py" line="168"/>
      <source>&amp;Annotation</source>
      <translation>&amp;Adnotacja</translation>
    </message>
    <message>
      <location filename="init_tools.py" line="170"/>
      <source>&amp;Modification</source>
      <translation>&amp;Modyfikacja</translation>
    </message>
    <message>
      <location filename="init_tools.py" line="172"/>
      <source>&amp;Utilities</source>
      <translation>&amp;Przybory</translation>
    </message>
    <message>
      <location filename="InitGui.py" line="122"/>
      <source>Draft</source>
      <translation>Szkic</translation>
    </message>
    <message>
      <location filename="InitGui.py" line="183"/>
      <source>Import-Export</source>
      <translation>Import-Eksport</translation>
    </message>
    <message>
      <location filename="pointarray.py" line="305"/>
      <source>Point object doesn't have a discrete point, it cannot be used for an array.</source>
      <translation>Obiekt punktu nie ma dyskretnego punktu, on nie może być użyty do tablicy.</translation>
    </message>
    <message>
      <location filename="bspline.py" line="106"/>
      <source>_BSpline.createGeometry: Closed with same first/last Point. Geometry not updated.</source>
      <translation>_BSpline.createGeometry: Zamknięta tylko samym Punktem pierwszym/końcowym. Geometria nie została zaktualizowana.</translation>
    </message>
    <message>
      <location filename="view_wpproxy.py" line="102"/>
      <source>Writing camera position</source>
      <translation>Pozycja kamery pisania</translation>
    </message>
    <message>
      <location filename="view_wpproxy.py" line="119"/>
      <source>Writing objects shown/hidden state</source>
      <translation>Zapisywanie obiektów pokazanych/ukrytych</translation>
    </message>
    <message>
      <location filename="view_layer.py" line="402"/>
      <source>Merge layer duplicates</source>
      <translation>Scal zduplikowane warstwy</translation>
    </message>
    <message>
      <location filename="view_layer.py" line="407"/>
      <source>Add new layer</source>
      <translation>Dodaj nową warstwę</translation>
    </message>
    <message>
      <location filename="init_draft_statusbar.py" line="244"/>
      <source>Toggles Grid On/Off</source>
      <translation>Włącz/wyłącz siatkę</translation>
    </message>
    <message>
      <location filename="init_draft_statusbar.py" line="262"/>
      <source>Object snapping</source>
      <translation>Przyciąganie obiektu</translation>
    </message>
    <message>
      <location filename="init_draft_statusbar.py" line="294"/>
      <source>Toggles Visual Aid Dimensions On/Off</source>
      <translation>Włącz/wyłącz wizualną pomoc wymiarów</translation>
    </message>
    <message>
      <location filename="init_draft_statusbar.py" line="314"/>
      <source>Toggles Ortho On/Off</source>
      <translation>Włącz/wyłącz Tryb ortogonalny</translation>
    </message>
    <message>
      <location filename="init_draft_statusbar.py" line="332"/>
      <source>Toggles Constrain to Working Plane On/Off</source>
      <translation type="unfinished">Toggles Constrain to Working Plane On/Off</translation>
    </message>
    <message>
      <location filename="gui_utils.py" line="144"/>
      <source>Unable to insert new object into a scaled part</source>
      <translation>Nie można wstawić nowego obiektu do części skalowanej</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="157"/>
      <source>True</source>
      <translation>Prawda</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="159"/>
      <source>False</source>
      <translation>Fałsz</translation>
    </message>
    <message>
      <location filename="task_scale.py" line="130"/>
      <source>Scale</source>
      <translation>Skala</translation>
    </message>
    <message>
      <location filename="task_scale.py" line="131"/>
      <source>X factor</source>
      <translation>Współczynnik X</translation>
    </message>
    <message>
      <location filename="task_scale.py" line="132"/>
      <source>Y factor</source>
      <translation>Współczynnik Y</translation>
    </message>
    <message>
      <location filename="task_scale.py" line="133"/>
      <source>Z factor</source>
      <translation>Współczynnik Z</translation>
    </message>
    <message>
      <location filename="task_scale.py" line="134"/>
      <source>Uniform scaling</source>
      <translation>Jednolite skalowanie</translation>
    </message>
    <message>
      <location filename="task_scale.py" line="135"/>
      <source>Working plane orientation</source>
      <translation>Orientacja płaszczyzny roboczej</translation>
    </message>
    <message>
      <location filename="task_scale.py" line="136"/>
      <source>Copy</source>
      <translation>Kopiuj</translation>
    </message>
    <message>
      <location filename="task_scale.py" line="137"/>
      <source>Modify subelements</source>
      <translation>Modyfikuj podelementy</translation>
    </message>
    <message>
      <location filename="task_scale.py" line="138"/>
      <source>Pick from/to points</source>
      <translation>Wybierz z/do punktów</translation>
    </message>
    <message>
      <location filename="task_scale.py" line="139"/>
      <source>Create a clone</source>
      <translation>Utwórz klon</translation>
    </message>
    <message>
      <location filename="gui_clone.py" line="91"/>
      <source>Clone</source>
      <translation>Klon</translation>
    </message>
    <message>
      <location filename="gui_lineslope.py" line="91"/>
      <source>Slope</source>
      <translation>Nachylenie</translation>
    </message>
    <message>
      <location filename="gui_circulararray.py" line="66"/>
      <source>Circular array</source>
      <translation>Szyk kołowy</translation>
    </message>
    <message>
      <location filename="gui_circulararray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in a radial pattern
creating various circular layers.

The array can be turned into an orthogonal or a polar array by changing its type.</source>
      <translation type="unfinished">Creates copies of the selected object, and places the copies in a radial pattern
creating various circular layers.

The array can be turned into an orthogonal or a polar array by changing its type.</translation>
    </message>
    <message>
      <location filename="gui_polararray.py" line="66"/>
      <source>Polar array</source>
      <translation>Szyk kołowy</translation>
    </message>
    <message>
      <location filename="gui_polararray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in a polar pattern
defined by a center of rotation and its angle.

The array can be turned into an orthogonal or a circular array by changing its type.</source>
      <translation type="unfinished">Creates copies of the selected object, and places the copies in a polar pattern
defined by a center of rotation and its angle.

The array can be turned into an orthogonal or a circular array by changing its type.</translation>
    </message>
    <message>
      <location filename="gui_arrays.py" line="65"/>
      <source>Array tools</source>
      <translation>Narzędzia szyku</translation>
    </message>
    <message>
      <location filename="gui_arrays.py" line="68"/>
      <source>Create various types of arrays, including rectangular, polar, circular, path, and point</source>
      <translation>Twórz różne typy szyków, w tym prostokątne, biegunowe, kołowe, po ścieżce i punktowe</translation>
    </message>
    <message>
      <location filename="gui_orthoarray.py" line="66"/>
      <source>Array</source>
      <translation>Tablica</translation>
    </message>
    <message>
      <location filename="gui_orthoarray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in an orthogonal pattern,
meaning the copies follow the specified direction in the X, Y, Z axes.

The array can be turned into a polar or a circular array by changing its type.</source>
      <translation>Tworzy kopie wybranego obiektu i umieszcza kopie w ortogonalnym wzorze,
oznacza, że kopie przebiegają zgodnie z określonym kierunkiem w osiach X, Y, Z.

Tablicę można przekształcić w tablicę polarną lub okrągłą, zmieniając jej typ.</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="67"/>
      <source>Fillet</source>
      <translation>Zaokrąglenie</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="67"/>
      <source>Creates a fillet between two selected wires or edges.</source>
      <translation>Tworzy wypełnianie pomiędzy dwoma wybranymi liniami łamanymi lub krawędziami.</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="89"/>
      <source>Delete original objects</source>
      <translation>Usuń oryginalne obiekty</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="95"/>
      <source>Create chamfer</source>
      <translation>Utwórz fazkę</translation>
    </message>
    <message>
      <location filename="gui_offset.py" line="326"/>
      <source>Offset direction is not defined. Please move the mouse on either side of the object first to indicate a direction</source>
      <translation type="unfinished">Offset direction is not defined. Please move the mouse on either side of the object first to indicate a direction</translation>
    </message>
  </context>
  <context>
    <name>DraftCircularArrayTaskPanel</name>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="26"/>
      <source>Circular array</source>
      <translation>Szyk kołowy</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="57"/>
      <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
      <translation>Współrzędne punktu, przez który przechodzi oś obrotu.
Zmień kierunek samej osi w edytorze właściwości.</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="61"/>
      <source>Center of rotation</source>
      <translation>Środek obrotu</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="69"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="76"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="83"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="131"/>
      <source>Reset the coordinates of the center of rotation.</source>
      <translation>Resetuj współrzędne środka obrotu.</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="134"/>
      <source>Reset point</source>
      <translation>Zresetuj punkt</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="146"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Jeżeli opcja jest zaznaczona, obiekty wynikowe w tablicy zostaną połączone, jeżeli będą się stykać.
Działa to tylko wtedy, gdy opcja "Połącz tablicę" jest wyłączona.</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="150"/>
      <source>Fuse</source>
      <translation>Suma</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="157"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Jeśli opcja zostanie zaznaczona, utworzony zostanie "Szyk powiązań" zamiast zwykłego szyku. Szyk powiązań działa wydajniej gdy potrzeba utworzyć wiele kopii, jednak nie można go później scalić.</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="161"/>
      <source>Link array</source>
      <translation>Szyk liniowy</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="186"/>
      <source>Distance from one element in one ring of the array to the next element in the same ring.
It cannot be zero.</source>
      <translation>Odległość od jednego elementu w jednym pierścieniu szyku do następnego elementu w tym samym pierścieniu. 
Nie może być równa zero.</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="179"/>
      <source>Tangential distance</source>
      <translation>Odległość styczna</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="210"/>
      <source>Distance from one layer of objects to the next layer of objects.</source>
      <translation>Odległość od jednej warstwy obiektów do następnej warstwy obiektów.</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="203"/>
      <source>Radial distance</source>
      <translation>Odległość promieniowa</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="261"/>
      <source>The number of symmetry lines in the circular array.</source>
      <translation>Liczba linii symetrii w szyku kołowym.</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="250"/>
      <source>Number of circular layers or rings to create, including a copy of the original object.
It must be at least 2.</source>
      <translation>Liczba warstw kołowych lub pierścieni do utworzenia, w tym kopia oryginalnego obiektu.
Musi wynosić co najmniej 2.</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="254"/>
      <source>Number of circular layers</source>
      <translation>Liczba warstw kołowych</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="264"/>
      <source>Symmetry</source>
      <translation>Symetria</translation>
    </message>
    <message>
      <location filename="TaskPanel_CircularArray.ui" line="273"/>
      <source>(Placeholder for the icon)</source>
      <translation>(Miejsce na ikonę)</translation>
    </message>
  </context>
  <context>
    <name>DraftOrthoArrayTaskPanel</name>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="26"/>
      <source>Orthogonal array</source>
      <translation>Szyk prostopadły</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="44"/>
      <source>Distance between the elements in the Z direction.
Normally, only the Z value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Odległość między elementami w kierunku Z.
Zazwyczaj tylko wartość Z jest potrzebna; podanie pozostałych dwóch wartości spowoduje dodatkowe przesunięcie w danych kierunkach.
Przy wartościach ujemnych kopie tworzone będą w kierunku przeciwnym.</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="49"/>
      <source>Z intervals</source>
      <translation>Odstępy w kierunku Z</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="385"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="392"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="378"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="353"/>
      <source>Reset the distances.</source>
      <translation>Zresetuj odległości.</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="125"/>
      <source>Reset Z</source>
      <translation>Zresetuj Z</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="137"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Jeżeli opcja jest zaznaczona, obiekty wynikowe w tablicy zostaną połączone, jeżeli będą się stykać.
Działa to tylko wtedy, gdy opcja "Połącz tablicę" jest wyłączona.</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="141"/>
      <source>Fuse</source>
      <translation>Suma</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="148"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Jeśli opcja zostanie zaznaczona, utworzony zostanie "Szyk powiązań" zamiast zwykłego szyku. Szyk powiązań działa wydajniej gdy potrzeba utworzyć wiele kopii, jednak nie można go później scalić.</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="152"/>
      <source>Link array</source>
      <translation>Szyk liniowy</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="177"/>
      <source>(Placeholder for the icon)</source>
      <translation>(Miejsce na ikonę)</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="184"/>
      <source>Distance between the elements in the X direction.
Normally, only the X value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Odległość między elementami w kierunku X.
Zazwyczaj tylko wartość X jest potrzebna; podanie pozostałych dwóch wartości spowoduje dodatkowe przesunięcie w danych kierunkach.
Przy wartościach ujemnych kopie tworzone będą w kierunku przeciwnym.</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="189"/>
      <source>X intervals</source>
      <translation>Odstępy w kierunku X</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="265"/>
      <source>Reset X</source>
      <translation>Zresetuj X</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="275"/>
      <source>Distance between the elements in the Y direction.
Normally, only the Y value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Odległość między elementami w kierunku Y.
Zazwyczaj tylko wartość Y jest konieczna; pozostałe dwie wartości mogą spowodować dodatkowe przesunięcie w swoich kierunkach.
Wartości ujemne będą skutkować kopiami wykonanymi w przeciwnym kierunku.</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="280"/>
      <source>Y intervals</source>
      <translation>Odstępy w kierunku Y</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="356"/>
      <source>Reset Y</source>
      <translation>Zresetuj Y</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="366"/>
      <source>Number of elements in the array in the specified direction, including a copy of the original object.
The number must be at least 1 in each direction.</source>
      <translation>Liczba elementów tablicy we wskazanym kierunku, łącznie z kopią oryginalnego obiektu.
Liczba ta musi wynosić co najmniej 1 w każdym kierunku.</translation>
    </message>
    <message>
      <location filename="TaskPanel_OrthoArray.ui" line="370"/>
      <source>Number of elements</source>
      <translation>Liczba elementów</translation>
    </message>
  </context>
  <context>
    <name>DraftPolarArrayTaskPanel</name>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="26"/>
      <source>Polar array</source>
      <translation>Szyk kołowy</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="57"/>
      <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
      <translation>Współrzędne punktu, przez który przechodzi oś obrotu.
Zmień kierunek samej osi w edytorze właściwości.</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="61"/>
      <source>Center of rotation</source>
      <translation>Środek obrotu</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="69"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="115"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="122"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="131"/>
      <source>Reset the coordinates of the center of rotation.</source>
      <translation>Resetuj współrzędne środka obrotu.</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="134"/>
      <source>Reset point</source>
      <translation>Zresetuj punkt</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="146"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Jeżeli opcja jest zaznaczona, obiekty wynikowe w tablicy zostaną połączone, jeżeli będą się stykać.
Działa to tylko wtedy, gdy opcja "Połącz tablicę" jest wyłączona.</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="150"/>
      <source>Fuse</source>
      <translation>Suma</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="157"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Jeśli opcja zostanie zaznaczona, utworzony zostanie "Szyk powiązań" zamiast zwykłego szyku. Szyk powiązań działa wydajniej gdy potrzeba utworzyć wiele kopii, jednak nie można go później scalić.</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="161"/>
      <source>Link array</source>
      <translation>Szyk liniowy</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="187"/>
      <source>Sweeping angle of the polar distribution.
A negative angle produces a polar pattern in the opposite direction.
The maximum absolute value is 360 degrees.</source>
      <translation>Kąt zasięgu szyku biegunowego.
Przy ujemnej wartości kąta szyk zostanie utworzony w kierunku przeciwnym.
Maksymalna wartość bezwzględna wynosi 360 stopni.</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="180"/>
      <source>Polar angle</source>
      <translation>Kąt zakresu szyku</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="219"/>
      <source>Number of elements in the array, including a copy of the original object.
It must be at least 2.</source>
      <translation>Liczba elementów w tablicy, łącznie z kopią oryginalnego obiektu.
Musi wynosić co najmniej 2.</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="212"/>
      <source>Number of elements</source>
      <translation>Liczba elementów</translation>
    </message>
    <message>
      <location filename="TaskPanel_PolarArray.ui" line="235"/>
      <source>(Placeholder for the icon)</source>
      <translation>(Miejsce na ikonę)</translation>
    </message>
  </context>
  <context>
    <name>DraftShapeStringGui</name>
    <message>
      <location filename="TaskShapeString.ui" line="26"/>
      <source>ShapeString</source>
      <translation>Kształt z tekstu</translation>
    </message>
    <message>
      <location filename="TaskShapeString.ui" line="115"/>
      <source>Text to be made into ShapeString</source>
      <translation>Tekst do przekształcenia w ciąg kształtów</translation>
    </message>
    <message>
      <location filename="TaskShapeString.ui" line="122"/>
      <source>String</source>
      <translation>Ciąg znaków</translation>
    </message>
    <message>
      <location filename="TaskShapeString.ui" line="133"/>
      <source>Height</source>
      <translation>Wysokość</translation>
    </message>
    <message>
      <location filename="TaskShapeString.ui" line="140"/>
      <source>Height of the result</source>
      <translation>Wysokość wyniku</translation>
    </message>
    <message>
      <location filename="TaskShapeString.ui" line="66"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="TaskShapeString.ui" line="83"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="TaskShapeString.ui" line="90"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="TaskShapeString.ui" line="101"/>
      <source>Font file</source>
      <translation>Plik czcionki</translation>
    </message>
    <message>
      <location filename="TaskShapeString.ui" line="73"/>
      <source>Enter coordinates or select point with mouse.</source>
      <translation>Wprowadź współrzędne lub wybierz punkt myszką.</translation>
    </message>
    <message>
      <location filename="TaskShapeString.ui" line="186"/>
      <source>Reset 3d point selection</source>
      <translation>Zresetuj wybór punktu 3d</translation>
    </message>
    <message>
      <location filename="TaskShapeString.ui" line="192"/>
      <source>Reset Point</source>
      <translation>Zresetuj współrzędne punktu</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddConstruction</name>
    <message>
      <location filename="gui_groups.py" line="342"/>
      <source>Add to Construction group</source>
      <translation>Dodaj do grupy konstrukcyjnej</translation>
    </message>
    <message>
      <location filename="gui_groups.py" line="344"/>
      <source>Adds the selected objects to the construction group,
and changes their appearance to the construction style.
It creates a construction group if it doesn't exist.</source>
      <translation>Dodaje wybrane obiekty do grupy konstrukcyjnej,
i zmienia ich wygląd na styl konstrukcji.
Tworzy grupę konstrukcji, jeśli nie istniała.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddPoint</name>
    <message>
      <location filename="gui_line_add_delete.py" line="57"/>
      <source>Add point</source>
      <translation>Dodaj punkt</translation>
    </message>
    <message>
      <location filename="gui_line_add_delete.py" line="60"/>
      <source>Adds a point to an existing Wire or B-spline.</source>
      <translation>Dodaje punkt do istniejącego szkieletu lub B-spline.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddToGroup</name>
    <message>
      <location filename="gui_groups.py" line="67"/>
      <source>Ungroup</source>
      <translation>Rozgrupuj</translation>
    </message>
    <message>
      <location filename="gui_groups.py" line="72"/>
      <source>Move to group</source>
      <translation>Przenieś do grupy</translation>
    </message>
    <message>
      <location filename="gui_groups.py" line="74"/>
      <source>Moves the selected objects to an existing group, or removes them from any group.
Create a group first to use this tool.</source>
      <translation>Przenosi wybrane obiekty do istniejącej grupy lub usuwa je z dowolnej grupy.
Utwórz najpierw grupę, aby użyć tego narzędzia.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AnnotationStyleEditor</name>
    <message>
      <location filename="gui_annotationstyleeditor.py" line="80"/>
      <source>Annotation styles...</source>
      <translation>Styl adnotacji...</translation>
    </message>
  </context>
  <context>
    <name>Draft_ApplyStyle</name>
    <message>
      <location filename="gui_styles.py" line="47"/>
      <source>Apply current style</source>
      <translation>Zastosuj bieżący styl</translation>
    </message>
    <message>
      <location filename="gui_styles.py" line="50"/>
      <source>Applies the current style defined in the toolbar (line width and colors) to the selected objects and groups.</source>
      <translation>Stosuje bieżący styl zdefiniowany na pasku narzędzi (szerokość linii i kolory) do wybranych obiektów i grup.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Arc</name>
    <message>
      <location filename="gui_arcs.py" line="66"/>
      <source>Arc</source>
      <translation>Łuk</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="69"/>
      <source>Creates a circular arc by a center point and a radius.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Tworzy łuk kołowy za pomocą punktu środkowego i promienia.
CTRL, aby przyciągnąć, SHIFT, aby utworzyć wiązanie.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ArcTools</name>
    <message>
      <location filename="gui_arcs.py" line="598"/>
      <source>Arc tools</source>
      <translation>Narzędzia łuku</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="601"/>
      <source>Create various types of circular arcs.</source>
      <translation>Tworzenie różnego rodzaju okrągłych łuków.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Array</name>
    <message>
      <location filename="gui_array_simple.py" line="68"/>
      <source>Array</source>
      <translation>Tablica</translation>
    </message>
    <message>
      <location filename="gui_array_simple.py" line="71"/>
      <source>Creates an array from a selected object.
By default, it is a 2x2 orthogonal array.
Once the array is created its type can be changed
to polar or circular, and its properties can be modified.</source>
      <translation>Tworzy tablicę z wybranego obiektu.
Domyślnie jest to tablica ortogonalna 2x2.
Po utworzeniu tablicy jej typ może zostać zmieniony
na polarny lub okrągły, a jej właściwości mogą być modyfikowane.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AutoGroup</name>
    <message>
      <location filename="gui_groups.py" line="238"/>
      <source>Autogroup</source>
      <translation type="unfinished">Autogroup</translation>
    </message>
    <message>
      <location filename="gui_groups.py" line="241"/>
      <source>Select a group to add all Draft and Arch objects to.</source>
      <translation>Wybierz grupę, do której chcesz dodać wszystkie obiekty Projektowe i Arch.</translation>
    </message>
  </context>
  <context>
    <name>Draft_BSpline</name>
    <message>
      <location filename="gui_splines.py" line="60"/>
      <source>B-spline</source>
      <translation>B-spline</translation>
    </message>
    <message>
      <location filename="gui_splines.py" line="63"/>
      <source>Creates a multiple-point B-spline. CTRL to snap, SHIFT to constrain.</source>
      <translation>Tworzy wielopunktowe B-spline. CTRL aby przyciągnąć, SHIFT aby ograniczyć.</translation>
    </message>
  </context>
  <context>
    <name>Draft_BezCurve</name>
    <message>
      <location filename="gui_beziers.py" line="64"/>
      <source>Bezier curve</source>
      <translation>Krzywa Beziera</translation>
    </message>
    <message>
      <location filename="gui_beziers.py" line="67"/>
      <source>Creates an N-degree Bezier curve. The more points you pick, the higher the degree.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Tworzy krzywą Beziera N-stopniowego. Im więcej punktów wybierzesz, tym wyższy stopień.
CTRL, aby przyciągnąć, SHIFT, aby ograniczyć.</translation>
    </message>
  </context>
  <context>
    <name>Draft_BezierTools</name>
    <message>
      <location filename="gui_beziers.py" line="475"/>
      <source>Bezier tools</source>
      <translation>Narzędzia Beziera</translation>
    </message>
    <message>
      <location filename="gui_beziers.py" line="478"/>
      <source>Create various types of Bezier curves.</source>
      <translation>Utwórz różne typy krzywych Beziera.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Circle</name>
    <message>
      <location filename="gui_circles.py" line="79"/>
      <source>Circle</source>
      <translation>Okrąg</translation>
    </message>
    <message>
      <location filename="gui_circles.py" line="83"/>
      <source>Creates a circle (full circular arc).
CTRL to snap, ALT to select tangent objects.</source>
      <translation>Tworzy okrąg (pełny okrągły łuk).
CTRL, aby przyciągnąć, ALT, aby wybrać styczny obiekt.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Clone</name>
    <message>
      <location filename="gui_clone.py" line="70"/>
      <source>Clone</source>
      <translation>Klon</translation>
    </message>
    <message>
      <location filename="gui_clone.py" line="73"/>
      <source>Creates a clone of the selected objects.
The resulting clone can be scaled in each of its three directions.</source>
      <translation>Tworzy klon zaznaczonych obiektów.
Uzyskany klon może być skalowany w każdym z trzech kierunków.</translation>
    </message>
  </context>
  <context>
    <name>Draft_CloseLine</name>
    <message>
      <location filename="gui_lineops.py" line="124"/>
      <source>Close Line</source>
      <translation>Połącz linie</translation>
    </message>
    <message>
      <location filename="gui_lineops.py" line="125"/>
      <source>Closes the line being drawn, and finishes the operation.</source>
      <translation>Kończy operację i zamyka rysowaną linię.</translation>
    </message>
  </context>
  <context>
    <name>Draft_CubicBezCurve</name>
    <message>
      <location filename="gui_beziers.py" line="243"/>
      <source>Cubic bezier curve</source>
      <translation>Sześcienna krzywa beziera</translation>
    </message>
    <message>
      <location filename="gui_beziers.py" line="246"/>
      <source>Creates a Bezier curve made of 2nd degree (quadratic) and 3rd degree (cubic) segments. Click and drag to define each segment.
After the curve is created you can go back to edit each control point and set the properties of each knot.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Tworzy krzywą Beziera wykonaną z segmentów drugiego stopnia (kwadrat) i trzeciego stopnia (sześciokąt). Kliknij i przeciągnij, aby zdefiniować każdy segment.
Po utworzeniu krzywej możesz wrócić do edycji każdego punktu kontrolnego i ustawić właściwości każdego węzła.
CTRL aby przyciągnąć, SHIFT aby utworzyć wiązanie.</translation>
    </message>
  </context>
  <context>
    <name>Draft_DelPoint</name>
    <message>
      <location filename="gui_line_add_delete.py" line="89"/>
      <source>Remove point</source>
      <translation>Usuń punkt</translation>
    </message>
    <message>
      <location filename="gui_line_add_delete.py" line="92"/>
      <source>Removes a point from an existing Wire or B-spline.</source>
      <translation type="unfinished">Removes a point from an existing Wire or B-spline.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Dimension</name>
    <message>
      <location filename="gui_dimensions.py" line="83"/>
      <source>Dimension</source>
      <translation>Wymiar</translation>
    </message>
    <message>
      <location filename="gui_dimensions.py" line="86"/>
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
      <translation type="unfinished">Creates a dimension.

- Pick three points to create a simple linear dimension.
- Select a straight line to create a linear dimension linked to that line.
- Select an arc or circle to create a radius or diameter dimension linked to that arc.
- Select two straight lines to create an angular dimension between them.
CTRL to snap, SHIFT to constrain, ALT to select an edge or arc.

You may select a single line or single circular arc before launching this command
to create the corresponding linked dimension.
You may also select an 'App::MeasureDistance' object before launching this command
to turn it into a 'Draft Dimension' object.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Downgrade</name>
    <message>
      <location filename="gui_downgrade.py" line="58"/>
      <source>Downgrade</source>
      <translation type="unfinished">Downgrade</translation>
    </message>
    <message>
      <location filename="gui_downgrade.py" line="61"/>
      <source>Downgrades the selected objects into simpler shapes.
The result of the operation depends on the types of objects, which may be able to be downgraded several times in a row.
For example, it explodes the selected polylines into simpler faces, wires, and then edges. It can also subtract faces.</source>
      <translation type="unfinished">Downgrades the selected objects into simpler shapes.
The result of the operation depends on the types of objects, which may be able to be downgraded several times in a row.
For example, it explodes the selected polylines into simpler faces, wires, and then edges. It can also subtract faces.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Draft2Sketch</name>
    <message>
      <location filename="gui_draft2sketch.py" line="57"/>
      <source>Draft to Sketch</source>
      <translation>Rysunek do szkicu</translation>
    </message>
    <message>
      <location filename="gui_draft2sketch.py" line="60"/>
      <source>Convert bidirectionally between Draft objects and Sketches.
Many Draft objects will be converted into a single non-constrained Sketch.
However, a single sketch with disconnected traces will be converted into several individual Draft objects.</source>
      <translation>Konwertuj dwukierunkowo pomiędzy obiektami Rysunków a szkicami.
Wiele obiektów Rysunków zostanie przekształconych w pojedynczy nieograniczony szkic.
Jednak pojedynczy szkic z rozłączonymi śladami zostanie przekonwertowany na kilka indywidualnych obiektów rysunkowych.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Drawing</name>
    <message>
      <location filename="gui_drawing.py" line="71"/>
      <source>Drawing</source>
      <translation>Rysunek</translation>
    </message>
    <message>
      <location filename="gui_drawing.py" line="74"/>
      <source>Creates a 2D projection on a Drawing Workbench page from the selected objects.
This command is OBSOLETE since the Drawing Workbench became obsolete in 0.17.
Use TechDraw Workbench instead for generating technical drawings.</source>
      <translation type="unfinished">Creates a 2D projection on a Drawing Workbench page from the selected objects.
This command is OBSOLETE since the Drawing Workbench became obsolete in 0.17.
Use TechDraw Workbench instead for generating technical drawings.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Edit</name>
    <message>
      <location filename="gui_edit.py" line="275"/>
      <source>Edit</source>
      <translation>Edytuj</translation>
    </message>
    <message>
      <location filename="gui_edit.py" line="284"/>
      <source>Edits the active object.
Press E or ALT+LeftClick to display context menu
on supported nodes and on supported objects.</source>
      <translation type="unfinished">Edits the active object.
Press E or ALT+LeftClick to display context menu
on supported nodes and on supported objects.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Ellipse</name>
    <message>
      <location filename="gui_ellipses.py" line="59"/>
      <source>Ellipse</source>
      <translation>Elipsa</translation>
    </message>
    <message>
      <location filename="gui_ellipses.py" line="62"/>
      <source>Creates an ellipse. CTRL to snap.</source>
      <translation>Tworzy elipsę. CTRL do przyciągania.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Facebinder</name>
    <message>
      <location filename="gui_facebinders.py" line="60"/>
      <source>Facebinder</source>
      <translation type="unfinished">Facebinder</translation>
    </message>
    <message>
      <location filename="gui_facebinders.py" line="62"/>
      <source>Creates a facebinder object from selected faces.</source>
      <translation>Tworzy obiekt powiązania ścian z pośród wybranych ścian.</translation>
    </message>
  </context>
  <context>
    <name>Draft_FinishLine</name>
    <message>
      <location filename="gui_lineops.py" line="98"/>
      <source>Finish line</source>
      <translation>Zakończ linię</translation>
    </message>
    <message>
      <location filename="gui_lineops.py" line="99"/>
      <source>Finishes a line without closing it.</source>
      <translation>Kończy linię bez jej zamykania.</translation>
    </message>
  </context>
  <context>
    <name>Draft_FlipDimension</name>
    <message>
      <location filename="gui_dimension_ops.py" line="62"/>
      <source>Flip dimension</source>
      <translation>Odwróć wymiar</translation>
    </message>
    <message>
      <location filename="gui_dimension_ops.py" line="66"/>
      <source>Flip the normal direction of the selected dimensions (linear, radial, angular).
If other objects are selected they are ignored.</source>
      <translation>Odwróć normalny kierunek wybranych wymiarów (liniowy, radialny, kątowy).
Jeśli zostaną wybrane inne obiekty to są one ignorowane.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Heal</name>
    <message>
      <location filename="gui_heal.py" line="58"/>
      <source>Heal</source>
      <translation>Wylecz</translation>
    </message>
    <message>
      <location filename="gui_heal.py" line="61"/>
      <source>Heal faulty Draft objects saved with an earlier version of the program.
If an object is selected it will try to heal that object in particular,
otherwise it will try to heal all objects in the active document.</source>
      <translation>Napraw wadliwe obiekty Szkiców zapisane z wcześniejszą wersją programu.
Jeśli obiekt jest zaznaczony to będzie próba naprawy tego obiektu w szczególności,
w przeciwnym razie spróbuje się naprawić wszystkie obiekty w aktywnym dokumencie.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Join</name>
    <message>
      <location filename="gui_join.py" line="66"/>
      <source>Join</source>
      <translation>Połącz</translation>
    </message>
    <message>
      <location filename="gui_join.py" line="69"/>
      <source>Joins the selected lines or polylines into a single object.
The lines must share a common point at the start or at the end for the operation to succeed.</source>
      <translation>Przyłącza zaznaczone linie lub polilinie do jednego obiektu.
Linie muszą mieć wspólny punkt na początku lub na końcu aby operacja odniosła sukces.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Label</name>
    <message>
      <location filename="gui_labels.py" line="64"/>
      <source>Label</source>
      <translation>Etykieta</translation>
    </message>
    <message>
      <location filename="gui_labels.py" line="67"/>
      <source>Creates a label, optionally attached to a selected object or subelement.

First select a vertex, an edge, or a face of an object, then call this command,
and then set the position of the leader line and the textual label.
The label will be able to display information about this object, and about the selected subelement,
if any.

If many objects or many subelements are selected, only the first one in each case
will be used to provide information to the label.</source>
      <translation>Tworzy etykietę, opcjonalnie dołączoną do wybranego obiektu lub jego części.

Najpierw wybierz wierzchołek, lub krawędź obiektu, lub powierzchnię a następnie wywołaj to polecenie,
i następnie ustaw pozycję lidera i etykietę tekstową.
Etykieta będzie mogła wyświetlać informacje o tym obiekcie, oraz o wybranej części obiektu, 
jeśli istnieje.

Jeśli wybrano wiele obiektów lub wiele części, tylko pierwszy w każdym przypadku
zostanie wykorzystany do dostarczenia informacji na etykiecie.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Layer</name>
    <message>
      <location filename="gui_layers.py" line="52"/>
      <source>Layer</source>
      <translation>Warstwa</translation>
    </message>
    <message>
      <location filename="gui_layers.py" line="55"/>
      <source>Adds a layer to the document.
Objects added to this layer can share the same visual properties such as line color, line width, and shape color.</source>
      <translation>Dodaje warstwę do dokumentu.
Obiekty dodane do tej warstwy mogą mieć te same właściwości wizualne, takie jak kolor linii, szerokość linii i kolor kształtu.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Line</name>
    <message>
      <location filename="gui_lines.py" line="64"/>
      <source>Line</source>
      <translation>Linia</translation>
    </message>
    <message>
      <location filename="gui_lines.py" line="67"/>
      <source>Creates a 2-point line. CTRL to snap, SHIFT to constrain.</source>
      <translation>Tworzy linię 2-punktową. CTRL do przyciągania, SHIFT do ograniczenia.</translation>
    </message>
  </context>
  <context>
    <name>Draft_LinkArray</name>
    <message>
      <location filename="gui_array_simple.py" line="119"/>
      <source>LinkArray</source>
      <translation type="unfinished">LinkArray</translation>
    </message>
    <message>
      <location filename="gui_array_simple.py" line="122"/>
      <source>Like the Array tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation type="unfinished">Like the Array tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Mirror</name>
    <message>
      <location filename="gui_mirror.py" line="63"/>
      <source>Mirror</source>
      <translation>Odbicie lustrzane</translation>
    </message>
    <message>
      <location filename="gui_mirror.py" line="66"/>
      <source>Mirrors the selected objects along a line defined by two points.</source>
      <translation>Odbija wybrane obiekty względem linii określonej przez dwa punkty.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Move</name>
    <message>
      <location filename="gui_move.py" line="63"/>
      <source>Move</source>
      <translation>Przesuń</translation>
    </message>
    <message>
      <location filename="gui_move.py" line="66"/>
      <source>Moves the selected objects from one base point to another point.
If the "copy" option is active, it will create displaced copies.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Przenosi zaznaczone obiekty z jednego punktu bazowego do innego punktu.
Jeśli opcja "kopiuj" jest aktywna, to stworzy kopię przeniesionych.
CTRL do przyciągania, SHIFT do ograniczenia.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Offset</name>
    <message>
      <location filename="gui_offset.py" line="63"/>
      <source>Offset</source>
      <translation>Odsunięcie</translation>
    </message>
    <message>
      <location filename="gui_offset.py" line="66"/>
      <source>Offsets of the selected object.
It can also create an offset copy of the original object.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
      <translation>Odsunięcia od wybranego obiektu.
Może również tworzyć odsuniętą kopię oryginalnego obiektu.
CTRL, aby przyciągnąć, SHIFT, aby ograniczyć. Przytrzymaj ALT i kliknij, aby utworzyć kopię za każdym kliknięciem.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathArray</name>
    <message>
      <location filename="gui_patharray.py" line="73"/>
      <source>Path array</source>
      <translation type="unfinished">Path array</translation>
    </message>
    <message>
      <location filename="gui_patharray.py" line="76"/>
      <source>Creates copies of the selected object along a selected path.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
      <translation type="unfinished">Creates copies of the selected object along a selected path.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathLinkArray</name>
    <message>
      <location filename="gui_patharray.py" line="163"/>
      <source>Path Link array</source>
      <translation type="unfinished">Path Link array</translation>
    </message>
    <message>
      <location filename="gui_patharray.py" line="166"/>
      <source>Like the PathArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation type="unfinished">Like the PathArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathTwistedArray</name>
    <message>
      <location filename="gui_pathtwistedarray.py" line="66"/>
      <source>Path twisted array</source>
      <translation type="unfinished">Path twisted array</translation>
    </message>
    <message>
      <location filename="gui_pathtwistedarray.py" line="69"/>
      <source>Creates copies of the selected object along a selected path, and twists the copies.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
      <translation type="unfinished">Creates copies of the selected object along a selected path, and twists the copies.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathTwistedLinkArray</name>
    <message>
      <location filename="gui_pathtwistedarray.py" line="121"/>
      <source>Path twisted Link array</source>
      <translation type="unfinished">Path twisted Link array</translation>
    </message>
    <message>
      <location filename="gui_pathtwistedarray.py" line="124"/>
      <source>Like the PathTwistedArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation type="unfinished">Like the PathTwistedArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Point</name>
    <message>
      <location filename="gui_points.py" line="62"/>
      <source>Point</source>
      <translation>Punkt</translation>
    </message>
    <message>
      <location filename="gui_points.py" line="65"/>
      <source>Creates a point object. Click anywhere on the 3D view.</source>
      <translation type="unfinished">Creates a point object. Click anywhere on the 3D view.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PointArray</name>
    <message>
      <location filename="gui_pointarray.py" line="76"/>
      <source>Point array</source>
      <translation>Szyk punktów</translation>
    </message>
    <message>
      <location filename="gui_pointarray.py" line="79"/>
      <source>Creates copies of the selected object, and places the copies at the position of various points.

The points need to be grouped under a compound of points before using this tool.
To create this compound, select various points and then use the Part Compound tool,
or use the Draft Upgrade tool to create a 'Block', or create a Sketch and add simple points to it.

Select the base object, and then select the compound or the sketch to create the point array.</source>
      <translation type="unfinished">Creates copies of the selected object, and places the copies at the position of various points.

The points need to be grouped under a compound of points before using this tool.
To create this compound, select various points and then use the Part Compound tool,
or use the Draft Upgrade tool to create a 'Block', or create a Sketch and add simple points to it.

Select the base object, and then select the compound or the sketch to create the point array.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PointLinkArray</name>
    <message>
      <location filename="gui_pointarray.py" line="142"/>
      <source>PointLinkArray</source>
      <translation type="unfinished">PointLinkArray</translation>
    </message>
    <message>
      <location filename="gui_pointarray.py" line="145"/>
      <source>Like the PointArray tool, but creates a 'Point link array' instead.
A 'Point link array' is more efficient when handling many copies.</source>
      <translation>Podobnie jak narzędzie PointArray, ale zamiast tego tworzy tablicę „Link punktowy”.
Tablica „Link punktowy” jest bardziej efektywna przy obsłudze wielu kopii.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Polygon</name>
    <message>
      <location filename="gui_polygons.py" line="57"/>
      <source>Polygon</source>
      <translation>Wielokąt</translation>
    </message>
    <message>
      <location filename="gui_polygons.py" line="60"/>
      <source>Creates a regular polygon (triangle, square, pentagon, ...), by defining the number of sides and the circumscribed radius.
CTRL to snap, SHIFT to constrain</source>
      <translation>Tworzy wielokąt regularny (trójkąt, kwadrat, pięciokąt, ...), definiując liczbę boków i zakreślony promień.
CTRL aby przyciągnąć, SHIFT aby ograniczyć</translation>
    </message>
  </context>
  <context>
    <name>Draft_Rectangle</name>
    <message>
      <location filename="gui_rectangles.py" line="54"/>
      <source>Rectangle</source>
      <translation>Prostokąt</translation>
    </message>
    <message>
      <location filename="gui_rectangles.py" line="57"/>
      <source>Creates a 2-point rectangle. CTRL to snap.</source>
      <translation>Tworzy prostokąt 2-punktowy. CTRL do przyciągnięcia.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Rotate</name>
    <message>
      <location filename="gui_rotate.py" line="62"/>
      <source>Rotate</source>
      <translation>Obróć</translation>
    </message>
    <message>
      <location filename="gui_rotate.py" line="65"/>
      <source>Rotates the selected objects. Choose the center of rotation, then the initial angle, and then the final angle.
If the "copy" option is active, it will create rotated copies.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
      <translation>Obraca wybrane obiekty. Wybierz środek obrotu, następnie kąt początkowy, a następnie kąt końcowy.
Jeśli opcja "kopiuj" jest aktywna, to stworzy kopie obracane.
CTRL, aby przyciągnąć, SHIFT, aby ograniczyć. Przytrzymaj ALT i kliknij aby utworzyć kopię za każdym kliknięciem.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Scale</name>
    <message>
      <location filename="gui_scale.py" line="71"/>
      <source>Scale</source>
      <translation>Skala</translation>
    </message>
    <message>
      <location filename="gui_scale.py" line="74"/>
      <source>Scales the selected objects from a base point.
CTRL to snap, SHIFT to constrain, ALT to copy.</source>
      <translation>Skaluje zaznaczone obiekty z punktu bazowego.
CTRL, aby przyciągnąć, SHIFT, aby ograniczyć, ALT do kopiowania.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SelectGroup</name>
    <message>
      <location filename="gui_groups.py" line="165"/>
      <source>Select group</source>
      <translation>Wybierz grupę</translation>
    </message>
  </context>
  <context>
    <name>Draft_SelectPlane</name>
    <message>
      <location filename="gui_selectplane.py" line="65"/>
      <source>SelectPlane</source>
      <translation>Wybierz płaszczyznę</translation>
    </message>
    <message>
      <location filename="gui_selectplane.py" line="67"/>
      <source>Select the face of solid body to create a working plane on which to sketch Draft objects.
You may also select a three vertices or a Working Plane Proxy.</source>
      <translation>Wybierz powierzchnię bryły, aby utworzyć płaszczyznę roboczą, na której będziesz szkicować obiekty rysunkowe.
Możesz również wybrać trzy wierzchołki lub roboczą Proxy Płaszczyznę.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SetStyle</name>
    <message>
      <location filename="gui_setstyle.py" line="49"/>
      <source>Set style</source>
      <translation>Ustaw styl</translation>
    </message>
    <message>
      <location filename="gui_setstyle.py" line="51"/>
      <source>Sets default styles</source>
      <translation>Ustaw jako domyślny styl</translation>
    </message>
  </context>
  <context>
    <name>Draft_SetWorkingPlaneProxy</name>
    <message>
      <location filename="gui_planeproxy.py" line="50"/>
      <source>Create working plane proxy</source>
      <translation>Utwórz pośrednią płaszczyznę roboczą</translation>
    </message>
    <message>
      <location filename="gui_planeproxy.py" line="52"/>
      <source>Creates a proxy object from the current working plane.
Once the object is created double click it in the tree view to restore the camera position and objects' visibilities.
Then you can use it to save a different camera position and objects' states any time you need.</source>
      <translation>Tworzy obiekt przejściowy na bieżącej płaszczyźnie roboczej.
Po utworzeniu obiektu kliknij go dwukrotnie w widoku drzewa, aby przywrócić pozycję i widoki obiektów.
Następnie można go użyć do zapisania innej pozycji obrazu i stanów obiektów w dowolnym momencie.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Shape2DView</name>
    <message>
      <location filename="gui_shape2dview.py" line="61"/>
      <source>Shape 2D view</source>
      <translation>Widok 2D modelu</translation>
    </message>
    <message>
      <location filename="gui_shape2dview.py" line="64"/>
      <source>Creates a 2D projection of the selected objects on the XY plane.
The initial projection direction is the negative of the current active view direction.
You can select individual faces to project, or the entire solid, and also include hidden lines.
These projections can be used to create technical drawings with the TechDraw Workbench.</source>
      <translation>Tworzy rzut 2D wybranych obiektów na płaszczyźnie XY.
Początkowy kierunek rzutowania jest ujemny od bieżącego aktywnego kierunku widoku.
Możesz wybrać poszczególne widoki do projektu lub całej bryły, a także dołączyć ukryte linie.
Te rzuty mogą być wykorzystane do tworzenia technicznych rysunków przy pomocy narzędzia TechDraw.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ShapeString</name>
    <message>
      <location filename="gui_shapestrings.py" line="70"/>
      <source>Shape from text</source>
      <translation>Kształt z tekstu</translation>
    </message>
    <message>
      <location filename="gui_shapestrings.py" line="72"/>
      <source>Creates a shape from a text string by choosing a specific font and a placement.
The closed shapes can be used for extrusions and boolean operations.</source>
      <translation>Tworzy kształt z ciągu tekstu, wybierając określoną czcionkę i umiejscowienie.
Zamknięte kształty mogą być używane do przeprowadzania operacji wyciągania i operacji logicznych.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ShowSnapBar</name>
    <message>
      <location filename="gui_snaps.py" line="573"/>
      <source>Show snap toolbar</source>
      <translation>Pokaż pasek narzędzi przyciągania</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="576"/>
      <source>Show the snap toolbar if it is hidden.</source>
      <translation>Pokaż pasek narzędzi, jeśli jest ukryty.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Slope</name>
    <message>
      <location filename="gui_lineslope.py" line="70"/>
      <source>Set slope</source>
      <translation>Ustaw nachylenie</translation>
    </message>
    <message>
      <location filename="gui_lineslope.py" line="73"/>
      <source>Sets the slope of the selected line by changing the value of the Z value of one of its points.
If a polyline is selected, it will apply the slope transformation to each of its segments.

The slope will always change the Z value, therefore this command only works well for
straight Draft lines that are drawn in the XY plane. Selected objects that aren't single lines will be ignored.</source>
      <translation>Ustawia nachylenie wybranej linii poprzez zmianę wartości Z jednego z jej punktów.
Jeśli wybrano polilinię, zastosuje transformację nachylenia do każdego z jej segmentów. 

Nachylenie zawsze zmieni wartość Z, dlatego to polecenie działa dobrze tylko dla
prostych linii projektowych, które są rysowane w płaszczyźnie XY. Wybrane obiekty, które nie są pojedynczymi liniami zostaną zignorowane.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap</name>
    <message>
      <location filename="gui_snapper.py" line="1527"/>
      <source>Toggles Grid On/Off</source>
      <translation>Włącz/wyłącz siatkę</translation>
    </message>
    <message>
      <location filename="gui_snapper.py" line="1528"/>
      <source>Toggle Draft Grid</source>
      <translation>Przełącz siatkę szkicu</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Angle</name>
    <message>
      <location filename="gui_snaps.py" line="327"/>
      <source>Angle</source>
      <translation>Kąt</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="330"/>
      <source>Set snapping to points in a circular arc located at multiples of 30 and 45 degree angles.</source>
      <translation>Ustaw przyciąganie do punktów w łuku kołowym znajdujących się pod wielokrotnością kątów 30 i 45 stopni.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Center</name>
    <message>
      <location filename="gui_snaps.py" line="357"/>
      <source>Center</source>
      <translation>Wyśrodkowane</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="360"/>
      <source>Set snapping to the center of a circular arc.</source>
      <translation>Ustaw przyciąganie do środka łuku kołowego.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Dimensions</name>
    <message>
      <location filename="gui_snaps.py" line="509"/>
      <source>Show dimensions</source>
      <translation>Pokaż wymiary</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="512"/>
      <source>Show temporary linear dimensions when editing an object and using other snapping methods.</source>
      <translation>Pokaż tymczasowe wymiary liniowe podczas edycji obiektu i przy użyciu innych metod przyciągania.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Endpoint</name>
    <message>
      <location filename="gui_snaps.py" line="296"/>
      <source>Endpoint</source>
      <translation type="unfinished">Endpoint</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="299"/>
      <source>Set snapping to endpoints of an edge.</source>
      <translation>Ustaw przyciąganie do punktu końcowego krawędzi.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Extension</name>
    <message>
      <location filename="gui_snaps.py" line="387"/>
      <source>Extension</source>
      <translation type="unfinished">Extension</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="390"/>
      <source>Set snapping to the extension of an edge.</source>
      <translation>Ustaw przyciąganie do przedłużenia krawędzi.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Grid</name>
    <message>
      <location filename="gui_snaps.py" line="206"/>
      <source>Grid</source>
      <translation>Siatka</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="209"/>
      <source>Set snapping to the intersection of grid lines.</source>
      <translation>Ustaw przyciąganie na przecięcie linii siatki.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Intersection</name>
    <message>
      <location filename="gui_snaps.py" line="236"/>
      <source>Intersection</source>
      <translation>Przecięcie</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="239"/>
      <source>Set snapping to the intersection of edges.</source>
      <translation>Ustaw przyciąganie na przecięcie krawędzi.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Lock</name>
    <message>
      <location filename="gui_snaps.py" line="116"/>
      <source>Main snapping toggle On/Off</source>
      <translation>Włącz/wyłącz główną opcję przyciągania</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="119"/>
      <source>Activates or deactivates all snap methods at once.</source>
      <translation>Aktywuje lub wyłącza wszystkie metody przyciągania naraz.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Midpoint</name>
    <message>
      <location filename="gui_snaps.py" line="146"/>
      <source>Midpoint</source>
      <translation type="unfinished">Midpoint</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="149"/>
      <source>Set snapping to the midpoint of an edge.</source>
      <translation>Ustaw przyciąganie na punkt środkowy krawędzi.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Near</name>
    <message>
      <location filename="gui_snaps.py" line="417"/>
      <source>Nearest</source>
      <translation>Najbliższy</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="420"/>
      <source>Set snapping to the nearest point of an edge.</source>
      <translation>Ustaw przyciąganie na najbliższy punkt krawędzi.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Ortho</name>
    <message>
      <location filename="gui_snaps.py" line="448"/>
      <source>Orthogonal</source>
      <translation>Tylko poziomo lub pionowo</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="451"/>
      <source>Set snapping to a direction that is a multiple of 45 degrees from a point.</source>
      <translation>Ustaw kierunek przyciągania na wielokrotność 45 stopni od punktu.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Parallel</name>
    <message>
      <location filename="gui_snaps.py" line="266"/>
      <source>Parallel</source>
      <translation>Równolegle</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="269"/>
      <source>Set snapping to a direction that is parallel to an edge.</source>
      <translation>Ustaw przyciąganie w kierunku równoległym do krawędzi.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Perpendicular</name>
    <message>
      <location filename="gui_snaps.py" line="176"/>
      <source>Perpendicular</source>
      <translation type="unfinished">Perpendicular</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="179"/>
      <source>Set snapping to a direction that is perpendicular to an edge.</source>
      <translation>Ustaw przyciąganie w kierunku prostopadłym do krawędzi.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Special</name>
    <message>
      <location filename="gui_snaps.py" line="478"/>
      <source>Special</source>
      <translation>Specjalne</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="481"/>
      <source>Set snapping to the special points defined inside an object.</source>
      <translation>Ustaw przyciąganie na specjalne punkty zdefiniowane wewnątrz obiektu.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_WorkingPlane</name>
    <message>
      <location filename="gui_snaps.py" line="542"/>
      <source>Working plane</source>
      <translation>Płaszczyzna robocza</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="545"/>
      <source>Restricts snapping to a point in the current working plane.
If you select a point outside the working plane, for example, by using other snapping methods,
it will snap to that point's projection in the current working plane.</source>
      <translation>Ogranicza przyciąganie do punktu w aktualnej płaszczyźnie roboczej.
Jeśli wybierzesz punkt poza płaszczyzną roboczą, na przykład używając innych metod przyciągania,
to zostanie przyciągnięty do rzutu tego punktu w aktualnej płaszczyźnie roboczej.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Split</name>
    <message>
      <location filename="gui_split.py" line="56"/>
      <source>Split</source>
      <translation>Rozdziel</translation>
    </message>
    <message>
      <location filename="gui_split.py" line="59"/>
      <source>Splits the selected line or polyline into two independent lines
or polylines by clicking anywhere along the original object.
It works best when choosing a point on a straight segment and not a corner vertex.</source>
      <translation>Rozdziela zaznaczoną linię lub polilinię na dwie niezależne linie
lub polilinie klikając wszędzie wzdłuż oryginalnego obiektu.
To działa najlepiej, wybierając punkt w prostym segmencie, a nie wierzchołku.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Stretch</name>
    <message>
      <location filename="gui_stretch.py" line="64"/>
      <source>Stretch</source>
      <translation>Rozciągnij</translation>
    </message>
    <message>
      <location filename="gui_stretch.py" line="67"/>
      <source>Stretches the selected objects.
Select an object, then draw a rectangle to pick the vertices that will be stretched,
then draw a line to specify the distance and direction of stretching.</source>
      <translation>Rozciąga zaznaczone obiekty.
Wybierz obiekt, a następnie narysuj prostokąt, aby wybrać wierzchołki, które zostaną rozciągnięte,
następnie narysuj linię w celu określenia odległości i kierunku rozciągania.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SubelementHighlight</name>
    <message>
      <location filename="gui_subelements.py" line="62"/>
      <source>Subelement highlight</source>
      <translation>Podświetlenie części obiektu</translation>
    </message>
    <message>
      <location filename="gui_subelements.py" line="65"/>
      <source>Highlight the subelements of the selected objects, so that they can then be edited with the move, rotate, and scale tools.</source>
      <translation>Podświetl części z wybranych obiektów, aby można je było edytować za pomocą narzędzi ruchu, obracania i skalowania.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Text</name>
    <message>
      <location filename="gui_texts.py" line="60"/>
      <source>Text</source>
      <translation>Tekst</translation>
    </message>
    <message>
      <location filename="gui_texts.py" line="63"/>
      <source>Creates a multi-line annotation. CTRL to snap.</source>
      <translation>Tworzy adnotację wieloliniową. CTRL do przyciągnięcia.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleConstructionMode</name>
    <message>
      <location filename="gui_togglemodes.py" line="99"/>
      <source>Toggle construction mode</source>
      <translation>Przełącz tryb konstrukcyjny</translation>
    </message>
    <message>
      <location filename="gui_togglemodes.py" line="102"/>
      <source>Toggles the Construction mode.
When this is active, the following objects created will be included in the construction group, and will be drawn with the specified color and properties.</source>
      <translation>Przełącza na tryb konstrukcji.
Gdy to jest aktywne, następujące obiekty zostaną włączone do grupy konstrukcyjnej, i zostaną narysowane z określonym kolorem i właściwościami.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleContinueMode</name>
    <message>
      <location filename="gui_togglemodes.py" line="131"/>
      <source>Toggle continue mode</source>
      <translation>Przełącz w tryb kontynuowania</translation>
    </message>
    <message>
      <location filename="gui_togglemodes.py" line="133"/>
      <source>Toggles the Continue mode.
When this is active, any drawing tool that is terminated will automatically start again.
This can be used to draw several objects one after the other in succession.</source>
      <translation>Przełącza na tryb kontynuowania.
Gdy jest to aktywne, każde narzędzie do rysowania, które zostanie zakończone, zostanie automatycznie uruchomione ponownie.
To może być użyte do rysowania kilku obiektów jeden po drugim.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleDisplayMode</name>
    <message>
      <location filename="gui_togglemodes.py" line="166"/>
      <source>Toggle normal/wireframe display</source>
      <translation type="unfinished">Toggle normal/wireframe display</translation>
    </message>
    <message>
      <location filename="gui_togglemodes.py" line="168"/>
      <source>Switches the display mode of selected objects from flatlines to wireframe and back.
This is helpful to quickly visualize objects that are hidden by other objects.
This is intended to be used with closed shapes and solids, and doesn't affect open wires.</source>
      <translation>Przełącza tryb wyświetlania wybranych obiektów z płaskich linii na pręty i odwrotnie.
Jest to pomocne w szybkiej wizualizacji obiektów, które są ukryte przez inne obiekty.
Jest to przeznaczone do użytku z zamkniętymi kształtami i bryłami i nie ma wpływu na otwarte pręty.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleGrid</name>
    <message>
      <location filename="gui_grid.py" line="58"/>
      <source>Toggle grid</source>
      <translation>Przełącz siatkę</translation>
    </message>
    <message>
      <location filename="gui_grid.py" line="59"/>
      <source>Toggles the Draft grid on and off.</source>
      <translation>Włącza/Wyłącza siatkę projektu.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Trimex</name>
    <message>
      <location filename="gui_trimex.py" line="76"/>
      <source>Trimex</source>
      <translation type="unfinished">Trimex</translation>
    </message>
    <message>
      <location filename="gui_trimex.py" line="79"/>
      <source>Trims or extends the selected object, or extrudes single faces.
CTRL snaps, SHIFT constrains to current segment or to normal, ALT inverts.</source>
      <translation>Przycina lub wydłuża zaznaczony obiekt, lub wyciąga pojedyncze ściany.
CTRL przyciąga, SHIFT wiąże do bieżącego segmentu lub do normalnej, ALT odwraca.</translation>
    </message>
  </context>
  <context>
    <name>Draft_UndoLine</name>
    <message>
      <location filename="gui_lineops.py" line="151"/>
      <source>Undo last segment</source>
      <translation>Cofnij ostatni segment</translation>
    </message>
    <message>
      <location filename="gui_lineops.py" line="152"/>
      <source>Undoes the last drawn segment of the line being drawn.</source>
      <translation type="unfinished">Undoes the last drawn segment of the line being drawn.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Upgrade</name>
    <message>
      <location filename="gui_upgrade.py" line="58"/>
      <source>Upgrade</source>
      <translation>Ulepsz</translation>
    </message>
    <message>
      <location filename="gui_upgrade.py" line="61"/>
      <source>Upgrades the selected objects into more complex shapes.
The result of the operation depends on the types of objects, which may be able to be upgraded several times in a row.
For example, it can join the selected objects into one, convert simple edges into parametric polylines,
convert closed edges into filled faces and parametric polygons, and merge faces into a single face.</source>
      <translation>Ulepsza wybrane obiekty do bardziej złożonych kształtów.
Rezultat operacji zależy od typów obiektów, które mogą być wielokrotnie aktualizowane w szeregu.
Na przykład może dołączyć wybrane obiekty do innego, konwertować proste krawędzie na polilinie parametryczne,
konwertować zamknięte krawędzie na wypełnione powierzchnie i wielokąty parametryczne i scalać powierzchnie w jedną powierzchnię.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Wire</name>
    <message>
      <location filename="gui_lines.py" line="312"/>
      <source>Polyline</source>
      <translation>Linia łamana</translation>
    </message>
    <message>
      <location filename="gui_lines.py" line="315"/>
      <source>Creates a multiple-points line (polyline). CTRL to snap, SHIFT to constrain.</source>
      <translation>Tworzy wielopunktową linię (polilinię). CTRL, aby przyciąć, SHIFT, aby ograniczyć.</translation>
    </message>
  </context>
  <context>
    <name>Draft_WireToBSpline</name>
    <message>
      <location filename="gui_wire2spline.py" line="63"/>
      <source>Wire to B-spline</source>
      <translation>Połącz z B-splajnem</translation>
    </message>
    <message>
      <location filename="gui_wire2spline.py" line="66"/>
      <source>Converts a selected polyline to a B-spline, or a B-spline to a polyline.</source>
      <translation>Konwertuje zaznaczoną polilinię do B-spline lub B-spline do polilinii.</translation>
    </message>
  </context>
  <context>
    <name>Form</name>
    <message>
      <location filename="TaskSelectPlane.ui" line="14"/>
      <source>Working plane setup</source>
      <translation>Ustawienia płaszczyzny roboczej</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="20"/>
      <source>Select a face or working plane proxy or 3 vertices.
Or choose one of the options below</source>
      <translation>Wybierz ścianę lub przybliżoną płaszczyznę roboczą albo 3 wierzchołki.
Lub wybierz jedną z poniższych opcji</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="31"/>
      <source>Sets the working plane to the XY plane (ground plane)</source>
      <translation>Ustawia płaszczyznę roboczą na płaszczyznę XY (od przodu)</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="34"/>
      <source>Top (XY)</source>
      <translation>Góra (XY)</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="41"/>
      <source>Sets the working plane to the XZ plane (front plane)</source>
      <translation>Ustawia płaszczyznę roboczą na płaszczyznę XZ (od przodu)</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="44"/>
      <source>Front (XZ)</source>
      <translation>Przód (XZ)</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="51"/>
      <source>Sets the working plane to the YZ plane (side plane)</source>
      <translation>Ustawia płaszczyznę roboczą na płaszczyznę YZ (od boku)</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="54"/>
      <source>Side (YZ)</source>
      <translation>Bok (YZ)</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="61"/>
      <source>Sets the working plane facing the current view</source>
      <translation>Ustawia płaszczyznę roboczą skierowaną do bieżącego widoku</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="64"/>
      <source>Align to view</source>
      <translation>Wyrównaj do widoku</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="71"/>
      <source>The working plane will align to the current
view each time a command is started</source>
      <translation>Płaszczyzna robocza będzie ustawiona do aktualnego
widoku za każdym razem, gdy uruchamiane jest polecenie</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="78"/>
      <source>Automatic</source>
      <translation>Automatyczna</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="87"/>
      <source>An optional offset to give to the working plane
above its base position. Use this together with one
of the buttons above</source>
      <translation>Opcjonalne odsunięcie do płaszczyzny roboczej
powyżej pozycji bazowej. Użyj tej opcji razem z jednym
z przycisków powyżej</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="99"/>
      <source>Offset</source>
      <translation>Odsunięcie</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="118"/>
      <source>If this is selected, the working plane will be
centered on the current view when pressing one
of the buttons above</source>
      <translation>Jeśli opcja ta jest wybrana, płaszczyzna robocza
zostanie ustawiona centralnie na aktualnym widoku
po naciśnięciu jednego z powyższych przycisków</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="111"/>
      <source>Center plane on view</source>
      <translation>Wyśrodkuj płaszczyznę na widoku</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="135"/>
      <source>Or select a single vertex to move the current
working plane without changing its orientation.
Then, press the button below</source>
      <translation>Lub wybierz pojedynczy wierzchołek aby przesunąć bieżącą
płaszczyznę roboczą bez zmiany orientacji.
Następnie naciśnij przycisk poniżej</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="147"/>
      <source>Moves the working plane without changing its
orientation. If no point is selected, the plane
will be moved to the center of the view</source>
      <translation>Przenosi płaszczyznę roboczą bez zmiany jej orientacji. 
Jeśli nie zaznaczono żadnego punktu, płaszczyzna
zostanie przeniesiona do środka widoku</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="152"/>
      <source>Move working plane</source>
      <translation>Przesuń płaszczyznę roboczą</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="171"/>
      <source>The spacing between the smaller grid lines</source>
      <translation>Odstęp między mniejszymi liniami siatki</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="164"/>
      <source>Grid spacing</source>
      <translation>Odstępy siatki</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="191"/>
      <source>The number of squares between each main line of the grid</source>
      <translation>Liczba kwadratów pomiędzy każdą główną linią siatki</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="184"/>
      <source>Main line every</source>
      <translation>Główna linia co</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="207"/>
      <source>The distance at which a point can be snapped to
when approaching the mouse. You can also change this
value by using the [ and ] keys while drawing</source>
      <translation>Odległość, w której kursor myszki może być przyciągnięty do punktu
podczas jego zbliżania. Możesz również zmienić tę wartość
używając klawisza [ i ] podczas rysowania</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="212"/>
      <source>Snapping radius</source>
      <translation>Promień przyciągania</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="241"/>
      <source>Centers the view on the current working plane</source>
      <translation>Wyśrodkowuje widok na aktualnej płaszczyźnie roboczej</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="244"/>
      <source>Center view</source>
      <translation>Widok wyśrodkowany</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="251"/>
      <source>Resets the working plane to its previous position</source>
      <translation>Resetuje płaszczyznę roboczą do poprzedniej pozycji</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="254"/>
      <source>Previous</source>
      <translation>Poprzedni</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="219"/>
      <source>Grid extension</source>
      <translation>Rozszerzenie siatki</translation>
    </message>
    <message>
      <location filename="TaskSelectPlane.ui" line="226"/>
      <source> lines</source>
      <translation> linie</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="14"/>
      <source>Style settings</source>
      <translation>Ustawienia stylu</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="207"/>
      <source>Text color</source>
      <translation>Kolor tekstu</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="132"/>
      <source>Shape color</source>
      <translation>Kolor kształtu</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="50"/>
      <source>Line width</source>
      <translation>Szerokość linii</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="29"/>
      <source>The color of faces</source>
      <translation>Kolor ścian</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="168"/>
      <source>The type of dimension arrows</source>
      <translation>Rodzaj strzałek linii wymiarowych</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="172"/>
      <source>Dot</source>
      <translation>Kropka</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="177"/>
      <source>Circle</source>
      <translation>Okrąg</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="182"/>
      <source>Arrow</source>
      <translation>Strzałka</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="187"/>
      <source>Tick</source>
      <translation>Odhacz</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="192"/>
      <source>Tick-2</source>
      <translation>Odhacz-2</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="234"/>
      <source>The color of texts and dimension texts</source>
      <translation>Kolor tekstów i wymiarów</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="241"/>
      <source>The size of texts and dimension texts</source>
      <translation>Rozmiar tekstów i wymiarów</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="281"/>
      <source>Show unit</source>
      <translation>Wyświetlaj jednostki</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="43"/>
      <source>Line color</source>
      <translation>Kolor linii</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="271"/>
      <source>The size of dimension arrows</source>
      <translation>Rozmiar strzałek linii wymiarowych</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="220"/>
      <source>The font to use for texts and dimensions</source>
      <translation>Czcionka używana dla tekstów i wymiarów</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="64"/>
      <source>The line style</source>
      <translation>Styl linii</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="68"/>
      <source>Solid</source>
      <translation>Bryła</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="73"/>
      <source>Dashed</source>
      <translation>Kreskowana</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="78"/>
      <source>Dotted</source>
      <translation>Kropkowana</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="83"/>
      <source>DashDot</source>
      <translation>KreskaKropka</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="227"/>
      <source>Text size</source>
      <translation>Rozmiar tekstu</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="295"/>
      <source>Unit override</source>
      <translation>Zastąpienie jednostki</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="302"/>
      <source>The unit to use for dimensions. Leave blank to use current FreeCAD unit</source>
      <translation>Jednostka używana do wymiarowania. Pozostaw to pole puste, aby użyć aktualnej jednostki FreeCAD</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="146"/>
      <source>The transparency of faces</source>
      <translation>Przezroczystość ścian</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="149"/>
      <source> %</source>
      <translation> %</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="139"/>
      <source>Transparency</source>
      <translation>Przezroczystość</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="98"/>
      <source>Display mode</source>
      <translation>Tryb wyświetlania</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="200"/>
      <source>Text font</source>
      <translation>Czcionka tekstu</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="288"/>
      <source>Arrow size</source>
      <translation>Rozmiar strzałki</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="105"/>
      <source>The display mode for faces</source>
      <translation>Tryb wyświetlania ścian</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="109"/>
      <source>Flat Lines</source>
      <translation>Cieniowany z krawędziami</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="114"/>
      <source>Wireframe</source>
      <translation>Szkieletowy</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="119"/>
      <source>Shaded</source>
      <translation>Cieniowany</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="124"/>
      <source>Points</source>
      <translation>Punkty</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="36"/>
      <source>Draw style</source>
      <translation>Styl rysowania</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="57"/>
      <source>The color of lines</source>
      <translation>Kolor linii</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="251"/>
      <source>Arrow style</source>
      <translation>Styl strzałki</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="91"/>
      <source> px</source>
      <translation> px</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="312"/>
      <source>Apply to selected objects</source>
      <translation>Zastosuj do wybranych obiektów</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="20"/>
      <source>Lines and faces</source>
      <translation>Linie i powierzchnie</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="162"/>
      <source>Annotations</source>
      <translation>Adnotacje</translation>
    </message>
    <message>
      <location filename="TaskPanel_SetStyle.ui" line="258"/>
      <source>If the unit suffix is shown on dimension texts or not</source>
      <translation>Opcja definiuje czy przyrostek jednostki jest pokazany na tekstach wymiarowych, czy nie</translation>
    </message>
  </context>
  <context>
    <name>Gui::Dialog::DlgSettingsDraft</name>
    <message>
      <location filename="preferences-draft.ui" line="32"/>
      <source>General Draft Settings</source>
      <translation>Ustawienia ogólne Środowiska pracy Draft</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="427"/>
      <source>This is the default color for objects being drawn while in construction mode.</source>
      <translation>Jest to domyślny kolor dla obiektów, które są sporządzone w trybie budowy.</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="387"/>
      <source>This is the default group name for construction geometry</source>
      <translation>Jest to domyślna nazwa grupy dla geometrii konstrukcji</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="390"/>
      <source>Construction</source>
      <translation>Konstrukcja</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="114"/>
      <source>Save current color and linewidth across sessions</source>
      <translation>Zapisz bieżący kolor i szerokość linii całej sesji</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="304"/>
      <source>Global copy mode</source>
      <translation>Tryb globalny kopiowania</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="40"/>
      <source>Default working plane</source>
      <translation>Domyślna płaszczyzna robocza</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="67"/>
      <source>None</source>
      <translation>Brak</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="72"/>
      <source>XY (Top)</source>
      <translation>XY (od góry)</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="77"/>
      <source>XZ (Front)</source>
      <translation>XZ (od przodu)</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="82"/>
      <source>YZ (Side)</source>
      <translation>YZ (od boku)</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="89"/>
      <source>Default height for texts and dimensions</source>
      <translation>Domyślna wysokość tekstów i wymiarów</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="47"/>
      <source>This is the default font name for all Draft texts and dimensions.
It can be a font name such as "Arial", a default style such as "sans", "serif"
or "mono", or a family such as "Arial,Helvetica,sans" or a name with a style
such as "Arial:Bold"</source>
      <translation>To jest nazwa domyślnej czcionki dla wszystkich tekstów i wymiarów. Może to być np. nazwa czcionki "Arial", domyślny styl, tak jak "sans", "serif" lub "mono" lub rodziny, jak np. "Arial, Helvetica, sans" lub nazwa w stylu np. "Arial: Bold"</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="151"/>
      <source>Default template sheet</source>
      <translation>Domyślny szablon arkusza</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="177"/>
      <source>The default template to use when creating a new drawing sheet</source>
      <translation>Domyślny szablon podczas tworzenia nowego arkusza rysunkowego</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="40"/>
      <source>Import style</source>
      <translation>Import stylu</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="60"/>
      <source>None (fastest)</source>
      <translation>Brak (najszybszy)</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="65"/>
      <source>Use default color and linewidth</source>
      <translation>Użyj domyślnego koloru i szerokości linii</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="70"/>
      <source>Original color and linewidth</source>
      <translation>Oryginalny kolor i szerokość linii</translation>
    </message>
    <message>
      <location filename="preferences-oca.ui" line="46"/>
      <source>Check this if you want the areas (3D faces) to be imported too.</source>
      <translation>Zaznacz tę opcję, jeśli chcesz, aby obszary (powierzchnie 3D) również były importowane.</translation>
    </message>
    <message>
      <location filename="preferences-oca.ui" line="49"/>
      <source>Import OCA areas</source>
      <translation>Import powierzchni OCA</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="14"/>
      <source>General settings</source>
      <translation>Ustawienia ogólne</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="380"/>
      <source>Construction group name</source>
      <translation>Nazwa grupy Konstrukcja</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="143"/>
      <source>Tolerance</source>
      <translation>Tolerancja</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="346"/>
      <source>Join geometry</source>
      <translation>Łącz geometrie</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="194"/>
      <source>Alternate SVG Patterns location</source>
      <translation>Alternatywna lokalizcja wzorców SVG</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="220"/>
      <source>Here you can specify a directory containing SVG files containing &lt;pattern&gt; definitions that can be added to the standard Draft hatch patterns</source>
      <translation>Podaj folder zawierający pliki SVG definicji wzorów wypełnienia ,które można dodać do standardowego modułu wypełnień Draft.</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="57"/>
      <source>Constrain mod</source>
      <translation>moduł Ograniczanie</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="77"/>
      <source>The Constraining modifier key</source>
      <translation>Klawisz modyfikujący ograniczanie</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="109"/>
      <source>Snap mod</source>
      <translation>Tryb przyciągania</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="129"/>
      <source>The snap modifier key</source>
      <translation>Klawisz modyfikujący przyciąganie</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="164"/>
      <source>Alt mod</source>
      <translation>Alt mod</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="287"/>
      <source>Select base objects after copying</source>
      <translation>Zaznacz obiekty bazowe po skopiowaniu</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="269"/>
      <source>If checked, a grid will appear when drawing</source>
      <translation>Jeśli zaznaczone, podczas rysowania będzie widoczna siatka</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="272"/>
      <source>Use grid</source>
      <translation>Użyj siatki</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="402"/>
      <source>Grid spacing</source>
      <translation>Odstępy siatki</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="425"/>
      <source>The spacing between each grid line</source>
      <translation>Odstępy między liniami siatki</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="353"/>
      <source>Main lines every</source>
      <translation>Wszystkie główne linie</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="376"/>
      <source>Mainlines will be drawn thicker. Specify here how many squares between mainlines.</source>
      <translation>Linia główna będzie grubsza. Określ, ile kwadratów między liniami głównymi.</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="94"/>
      <source>Internal precision level</source>
      <translation>Wewnętrzny poziom dokładności</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="395"/>
      <source>This is the orientation of the dimension texts when those dimensions are vertical. Default is left, which is the ISO standard.</source>
      <translation>Jest to orientacja tekstów wymiarów, gdy te wymiary są pionowe. Pozostawiona jest domyślna, która jest normą ISO.</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="405"/>
      <source>Left (ISO standard)</source>
      <translation>Lewo (standard ISO)</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="410"/>
      <source>Right</source>
      <translation>Prawo</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="367"/>
      <source>Group layers into blocks</source>
      <translation>Zgrupuj warstwy w Bloki</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="553"/>
      <source>Export 3D objects as polyface meshes</source>
      <translation>Eksport obiektu 3D jako wielofasetowa siatka</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="219"/>
      <source>If checked, the Snap toolbar will be shown whenever you use snapping</source>
      <translation>Jeśli to pole jest zaznaczone, pasek narzędzi Przyciąganie będzie wyświetlany przy każdym użyciu funkcji przyciągania</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="222"/>
      <source>Show Draft Snap toolbar</source>
      <translation>Pokaż pasek narzędzi przyciągania Środowiska pracy Draft</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="242"/>
      <source>Hide Draft snap toolbar after use</source>
      <translation>Ukryj pasek narzędziowy przyciągania Środowiska pracy Draft po użyciu</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="134"/>
      <source>Show Working Plane tracker</source>
      <translation>Pokaż tracker płaszczyzny pracy</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="291"/>
      <source>If checked, the Draft grid will always be visible when the Draft workbench is active. Otherwise only when using a command</source>
      <translation>Jeśli ta opcja jest zaznaczona, siatka modułu Draft zawsze będzie widoczna w czasie gdy moduł Draft jest modułem aktywnym. W przeciwnym razie tylko używając polecenia</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="388"/>
      <source>Use standard font size for texts</source>
      <translation>Użyj standardowego rozmiaru czcionki dla tekstów</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="431"/>
      <source>Import hatch boundaries as wires</source>
      <translation>Importuj obwiednie kreskowania jako linie łamane</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="452"/>
      <source>Render polylines with width</source>
      <translation>Renderuj polilinie o szerokości</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="141"/>
      <source>Translated (for print &amp; display)</source>
      <translation>Skonwertowane (dla druku i wyświetlania)</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="146"/>
      <source>Raw (for CAM)</source>
      <translation>Raw (dla CAM)</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="161"/>
      <source>Translate white line color to black</source>
      <translation>Zamień kolor linii z białego na czarny</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="324"/>
      <source>Use Part Primitives when available</source>
      <translation>Używaj Prymitywów Części, gdy dostępne</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="26"/>
      <source>Snapping</source>
      <translation>Przyciąganie</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="34"/>
      <source>If this is checked, snapping is activated without the need to press the snap mod key</source>
      <translation>Jeśli ta opcja jest zaznaczona, przyciąganie jest aktywowane bez konieczności naciskania przycisku przyciąganie</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="37"/>
      <source>Always snap (disable snap mod)</source>
      <translation>Zawsze przyciągaj (wyłącz tryb przyciągania)</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="407"/>
      <source>Construction geometry color</source>
      <translation>Kolor geometrii konstrukcji</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="132"/>
      <source>Import</source>
      <translation>Import</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="142"/>
      <source>texts and dimensions</source>
      <translation>Teksty i wymiary</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="158"/>
      <source>points</source>
      <translation>Punkty</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="174"/>
      <source>layouts</source>
      <translation>Układy</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="190"/>
      <source>*blocks</source>
      <translation>Bloki</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="597"/>
      <source>Project exported objects along current view direction</source>
      <translation>Projektuj wyeksportowane obiekty wzdłuż bieżącego kierunku widoku</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="14"/>
      <source>Visual settings</source>
      <translation>Ustawienia wyglądu</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="26"/>
      <source>Visual Settings</source>
      <translation>Ustawienia wyglądu</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="34"/>
      <source>Snap symbols style</source>
      <translation>Styl symboli przyciągania</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="54"/>
      <source>Draft classic style</source>
      <translation>Styl klasyczny szkicu</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="59"/>
      <source>Bitsnpieces style</source>
      <translation>Styl Bitsnpieces</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="80"/>
      <source>Color</source>
      <translation>Kolor</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="237"/>
      <source>Hatch patterns resolution</source>
      <translation>Rozdzielczość wzorów kreskowania</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="263"/>
      <source>Grid</source>
      <translation>Siatka</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="294"/>
      <source>Always show the grid</source>
      <translation>Zawsze pokazuj siatkę</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="14"/>
      <source>Texts and dimensions</source>
      <translation>Teksty i wymiary</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="59"/>
      <source>Internal font</source>
      <translation>Wewnętrzna czcionka</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="323"/>
      <source>Dot</source>
      <translation>Kropka</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="328"/>
      <source>Circle</source>
      <translation>Okrąg</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="333"/>
      <source>Arrow</source>
      <translation>Strzałka</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="362"/>
      <source>The default size of arrows</source>
      <translation>Domyślny rozmiar strzałek</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="208"/>
      <source>The default size of dimensions extension lines</source>
      <translation>Domyślny rozmiar linii pomocniczych</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="429"/>
      <source>The space between the dimension line and the dimension text</source>
      <translation>Przestrzeń między linią wymiarową i tekstem wymiarowym</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="534"/>
      <source>Select a font file</source>
      <translation>Wybierz plik czcionki</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="267"/>
      <source>Fill objects with faces whenever possible</source>
      <translation>Wypełnij obiekt powierzchniami, gdy tylko możliwe</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="207"/>
      <source>Create</source>
      <translation>Utwórz</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="217"/>
      <source>simple Part shapes</source>
      <translation>Podstawowe kształty Części</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="236"/>
      <source>Draft objects</source>
      <translation>Szkicuj obiekty</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="252"/>
      <source>Sketches</source>
      <translation>Szkice</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="325"/>
      <source>Get original colors from the DXF file</source>
      <translation>Pobierz oryginalne kolory z pliku DXF</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="472"/>
      <source>Treat ellipses and splines as polylines</source>
      <translation>Traktuj elipsy i spliny jako polilinie</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="121"/>
      <source>Export style</source>
      <translation>Eksportu stylu</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="455"/>
      <source>Show the unit suffix in dimensions</source>
      <translation>Pokaż przyrostek jednostki w wymiarach</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="93"/>
      <source>Allow FreeCAD to automatically download and update the DXF libraries</source>
      <translation>Pozwól programowi FreeCAD na automatyczne pobieranie i aktualizację bibliotek DXF</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="209"/>
      <source>mm</source>
      <translation>mm</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="457"/>
      <source>Grid size</source>
      <translation>Rozmiar siatki</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="483"/>
      <source> lines</source>
      <translation> linie</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="144"/>
      <source>text above (2D)</source>
      <translation>Tekst powyżej (2D)</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="149"/>
      <source> text inside (3D)</source>
      <translation> tekst wewnątrz (3D)</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="375"/>
      <source>Dashed line definition</source>
      <translation>Definicja linii przerywanej</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="398"/>
      <source>0.09,0.05</source>
      <translation>0.09,0.05</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="418"/>
      <source>Dashdot line definition</source>
      <translation>Definicja linii przerywanej</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="441"/>
      <source>0.09,0.05,0.02,0.05</source>
      <translation>0.09,0.05,0.02,0.05</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="461"/>
      <source>Dotted line definition</source>
      <translation>Definicja linii kropkowanej</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="484"/>
      <source>0.02,0.02</source>
      <translation>0.02,0.02</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="14"/>
      <source>Grid and snapping</source>
      <translation>Siatka i przyciąganie</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="26"/>
      <source>Text settings</source>
      <translation>Ustawienia dla tekstu</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="34"/>
      <source>Font family</source>
      <translation>Rodzina czcionek</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="76"/>
      <source>Font size</source>
      <translation>Wielkość czcionki</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="116"/>
      <source>Dimension settings</source>
      <translation>Ustawienia wymiaru</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="124"/>
      <source>Display mode</source>
      <translation>Tryb wyświetlania</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="309"/>
      <source>Arrows style</source>
      <translation>Styl strzałki</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="355"/>
      <source>Arrows size</source>
      <translation>Rozmiar strzałki</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="388"/>
      <source>Text orientation</source>
      <translation>Orientacja tekstu</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="422"/>
      <source>Text spacing</source>
      <translation>Odstępy w tekście</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="513"/>
      <source>ShapeString settings</source>
      <translation>Ustawienia dla Kształt z tekstu</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="521"/>
      <source>Default ShapeString font file</source>
      <translation>Domyślny plik czcionki dla Kształt z tekstu</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="367"/>
      <source>Drawing view line definitions</source>
      <translation>Rysowanie widoku definicji lini</translation>
    </message>
    <message>
      <location filename="preferences-dwg.ui" line="14"/>
      <source>DWG</source>
      <translation>DWG</translation>
    </message>
    <message>
      <location filename="preferences-dwg.ui" line="26"/>
      <source>DWG conversion</source>
      <translation>konwersja DWG</translation>
    </message>
    <message>
      <location filename="preferences-dwg.ui" line="56"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=" font-weight:600;"&gt;Note:&lt;/span&gt; DXF options apply to DWG files as well.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=" font-weight:600;"&gt;Wskazówka:&lt;/span&gt; Opcje DXF mają zastosowanie również do plików DWG.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="14"/>
      <source>DXF</source>
      <translation>DXF</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="26"/>
      <source>Import options</source>
      <translation>Opcje importu</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="46"/>
      <source>Use legacy python importer</source>
      <translation>Użyj starszego importera Python</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="107"/>
      <source>Export options</source>
      <translation>Opcje eksportu</translation>
    </message>
    <message>
      <location filename="preferences-oca.ui" line="14"/>
      <source>OCA</source>
      <translation>OCA</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="14"/>
      <source>SVG</source>
      <translation>SVG</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="86"/>
      <source>Disable units scaling</source>
      <translation>Wyłącz skalowanie jednostek</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="574"/>
      <source>Export Drawing Views as blocks</source>
      <translation>Eksportuj widoki rysunku jako bloki</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="123"/>
      <source>Note: Not all the options below are used by the new importer yet</source>
      <translation>Wskazówka: Nie wszystkie opcje prezentowane poniżej są używane przez nowego importera</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="29"/>
      <source>Show this dialog when importing and exporting</source>
      <translation>Pokaż okno dialogowe podczas importowania i eksportowania</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="80"/>
      <source>Automatic update (legacy importer only)</source>
      <translation>Automatyczna aktualizacja (tylko starszy importer)</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="341"/>
      <source>Prefix labels of Clones with:</source>
      <translation>Prefiksy etykiet klonów za pomocą:</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="272"/>
      <source>Scale factor to apply to imported files</source>
      <translation>Współczynnik skalowania do zastosowania przy imporcie plików</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="181"/>
      <source>Max segment length for discretized arcs</source>
      <translation>Maksymalna długość segmentu dla łuków dyskretnych</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="161"/>
      <source>Number of decimals</source>
      <translation>Ilość miejsc dziesiętnych</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="197"/>
      <source>Shift</source>
      <translation>Shift</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="202"/>
      <source>Ctrl</source>
      <translation>Ctrl</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="207"/>
      <source>Alt</source>
      <translation>Alt</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="184"/>
      <source>The Alt modifier key</source>
      <translation>Klawisz modyfikujący Alt</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="477"/>
      <source>The number of horizontal or vertical lines of the grid</source>
      <translation>Liczba poziomych lub pionowych linii sietki</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="87"/>
      <source>The default color for snap symbols</source>
      <translation>Kolor domyślny dla symboli przyciągania</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="111"/>
      <source>Check this if you want to use the color/linewidth from the toolbar as default</source>
      <translation>Zaznacz to, jeśli chcesz używać domyślnego koloru/szerokości linii na pasku narzędzi</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="131"/>
      <source>If checked, a widget indicating the current working plane orientation appears during drawing operations</source>
      <translation>Jeśli jest zaznaczone, podczas operacji rysowania pojawia się widżet wskazujący bieżącą orientację płaszczyzny roboczej</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="481"/>
      <source>An SVG linestyle definition</source>
      <translation>Definicja rodzaju linii SVG</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="201"/>
      <source>Extension lines size</source>
      <translation>Wielkość linii rozszerzeń</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="237"/>
      <source>Extension line overshoot</source>
      <translation>Przekroczenie linii rozszerzeń</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="244"/>
      <source>The default length of extension line above dimension line</source>
      <translation>Domyślna długość linii pomocniczej powyżej linii wymiarowej</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="273"/>
      <source>Dimension line overshoot</source>
      <translation>Przekroczenie linii wymiaru</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="280"/>
      <source>The default distance the dimension line is extended past extension lines</source>
      <translation>Domyślna odległość na jaką linia wymiarowa jest wydłużona poza linie rozszerzeń</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="338"/>
      <source>Tick</source>
      <translation>Odhacz</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="343"/>
      <source>Tick-2</source>
      <translation>Odhacz-2</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="326"/>
      <source>Check this if you want to preserve colors of faces while doing downgrade and upgrade (splitFaces and makeShell only)</source>
      <translation>Zaznacz to, jeśli chcesz zachować kolory ściany podczas wykonywania obniżania i podnoszenia (tylko przy dzieleniu ściany i tworzeniu powłoki)</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="329"/>
      <source>Preserve colors of faces during downgrade/upgrade</source>
      <translation>Zachowaj kolory powierzchni podczas obniżania / podnoszenia</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="346"/>
      <source>Check this if you want the face names to derive from the originating object name and vice versa while doing downgrade/upgrade (splitFaces and makeShell only)</source>
      <translation>Zaznacz to pole, jeśli chcesz, aby nazwy powierzchni pochodziły od nazwy obiektu źródłowego i podobnie podczas wykonywania aktualizacji i przywracania (tylko przy dzieleniu ściany i tworzeniu powłoki)</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="349"/>
      <source>Preserve names of faces during downgrade/upgrade</source>
      <translation>Zachowaj kolory powierzchni podczas obniżania / podnoszenia</translation>
    </message>
    <message>
      <location filename="preferences-dwg.ui" line="41"/>
      <source>The path to your ODA (formerly Teigha) File Converter executable</source>
      <translation>Ścieżka do twojego pliku konwertera ODA (wcześniej Teigha)</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="469"/>
      <source>Ellipse export is poorly supported. Use this to export them as polylines instead.</source>
      <translation>Eksport elipsy jest słabo wspierany. Użyj tej opcji, aby wyeksportować je jako polilinie.</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="501"/>
      <source>Max Spline Segment:</source>
      <translation>Maksymalny segment splajnu:</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="120"/>
      <source>The number of decimals in internal coordinates operations (for ex. 3 = 0.001). Values between 6 and 8 are usually considered the best trade-off among FreeCAD users.</source>
      <translation>Liczba miejsc po przecinku we współrzędnych wewnętrznych (np. 3 = 0,001). Wartości pomiędzy 6 a 8 są zwykle uważane za najlepszy kompromis wśród użytkowników FreeCADa.</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="169"/>
      <source>This is the value used by functions that use a tolerance.
Values with differences below this value will be treated as same. This value will be obsoleted soon so the precision level above controls both.</source>
      <translation>Jest to wartość używana przez funkcje, które używają tolerancji. Wartości z różnicami poniżej tej wartości będą traktowane tak samo. Ta wartość będzie wkrótce przestarzała, więc powyższy poziom dokładności kontroluje oba.</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="67"/>
      <source>Use legacy python exporter</source>
      <translation>Użyj starszego eksportera Python</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="247"/>
      <source>If this option is set, when creating Draft objects on top of an existing face of another object, the "Support" property of the Draft object will be set to the base object. This was the standard behaviour before FreeCAD 0.19</source>
      <translation>Jeśli ta opcja jest ustawiona, podczas tworzenia obiektów Szkicu na istniejącej powierzchni innego obiektu, właściwość "Wsparcie" obiektu Szkicu zostanie ustawiona na obiekt podstawowy. Było to standardowe zachowanie przed FreeCAD 0,19</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="372"/>
      <source>Construction Geometry</source>
      <translation>Geometria konstrukcyjna</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="26"/>
      <source>In-Command Shortcuts</source>
      <translation>Skróty poleceń</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="118"/>
      <source>Relative</source>
      <translation>Względny</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="140"/>
      <source>R</source>
      <translation>R</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="236"/>
      <source>Continue</source>
      <translation>Kontynuuj</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="214"/>
      <source>T</source>
      <translation>T</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="111"/>
      <source>Close</source>
      <translation>Zamknij</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="177"/>
      <source>O</source>
      <translation>O</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="243"/>
      <source>Copy</source>
      <translation>Kopiuj</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="469"/>
      <source>P</source>
      <translation>P</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="498"/>
      <source>Subelement Mode</source>
      <translation>Tryb podelementów</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="381"/>
      <source>D</source>
      <translation>D</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="491"/>
      <source>Fill</source>
      <translation>Wypełnij</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="425"/>
      <source>L</source>
      <translation>L</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="447"/>
      <source>Exit</source>
      <translation>Wyjdź</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="638"/>
      <source>A</source>
      <translation>A</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="579"/>
      <source>Select Edge</source>
      <translation>Wybierz krawędź</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="520"/>
      <source>E</source>
      <translation>E</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="403"/>
      <source>Add Hold</source>
      <translation>Dodaj uchwyt</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="675"/>
      <source>Q</source>
      <translation>Q</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="352"/>
      <source>Length</source>
      <translation>Długość</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="557"/>
      <source>H</source>
      <translation>H</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="359"/>
      <source>Wipe</source>
      <translation>Wyczyść</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="601"/>
      <source>W</source>
      <translation>W</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="697"/>
      <source>Set WP</source>
      <translation>Ustaw WP</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="719"/>
      <source>U</source>
      <translation>U</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="257"/>
      <source>Cycle Snap</source>
      <translation>Przełącz przyciąganie</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="52"/>
      <source>`</source>
      <translation>`</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="338"/>
      <source>Snap</source>
      <translation>Przyciągnij</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="89"/>
      <source>S</source>
      <translation>S</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="250"/>
      <source>Increase Radius</source>
      <translation>Zwiększ promień</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="279"/>
      <source>[</source>
      <translation>[</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="345"/>
      <source>Decrease Radius</source>
      <translation>Zmniejsz promień</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="316"/>
      <source>]</source>
      <translation>]</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="741"/>
      <source>Restrict X</source>
      <translation>Ogranicz X</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="763"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="785"/>
      <source>Restrict Y</source>
      <translation>Ogranicz Y</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="807"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="829"/>
      <source>Restrict Z</source>
      <translation>Ogranicz Z</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="851"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="194"/>
      <source>If this option is checked, the layers drop-down list will also show groups, allowing you to automatically add objects to groups too.</source>
      <translation>Jeśli ta opcja jest zaznaczona, lista rozwijana warstw wyświetli również grupy, co pozwoli ci automatycznie dodawać do nich obiekty.</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="197"/>
      <source>Show groups in layers list drop-down button</source>
      <translation>Pokaż grupy w rozwijanym przycisku listy warstw</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="216"/>
      <source>Draft tools options</source>
      <translation>Opcje narzędzi do szkicowania</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="230"/>
      <source>When drawing lines, set focus on Length instead of X coordinate.
This allows to point the direction and type the distance.</source>
      <translation>Podczas rysowania linii ustaw ostrość na długości zamiast na współrzędnej X.
Pozwala to wskazać kierunek i wprowadzić odległość.</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="234"/>
      <source>Set focus on Length instead of X coordinate</source>
      <translation>Ustaw ostrość na długość zamiast na współrzędną X</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="250"/>
      <source>Set the Support property when possible</source>
      <translation type="unfinished">Set the Support property when possible</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="263"/>
      <source>If this is checked, objects will appear as filled by default.
Otherwise, they will appear as wireframe</source>
      <translation>Jeśli ta opcja jest zaznaczona, obiekty pojawią się jako wypełnione domyślnie. W przeciwnym razie pojawią się one jako szkielet</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="283"/>
      <source>Normally, after copying objects, the copies get selected.
If this option is checked, the base objects will be selected instead.</source>
      <translation>Na ogół, po skopiowaniu obiektów, zostają wybrane kopie.
Jeśli ta opcja jest zaznaczona, to zamiast nich zostaną zaznaczone obiekty bazowe.</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="300"/>
      <source>If this is checked, copy mode will be kept across command,
otherwise commands will always start in no-copy mode</source>
      <translation type="unfinished">If this is checked, copy mode will be kept across command,
otherwise commands will always start in no-copy mode</translation>
    </message>
    <message>
      <location filename="preferences-draft.ui" line="320"/>
      <source>Force Draft Tools to create Part primitives instead of Draft objects.
Note that this is not fully supported, and many object will be not editable with Draft Modifiers.</source>
      <translation type="unfinished">Force Draft Tools to create Part primitives instead of Draft objects.
Note that this is not fully supported, and many object will be not editable with Draft Modifiers.</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="14"/>
      <source>User interface settings</source>
      <translation>Ustawienia interfejsu użytkownika</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="884"/>
      <source>Enable draft statusbar customization</source>
      <translation>Włącz dostosowywanie paska stanu szkicu roboczego</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="887"/>
      <source>Draft Statusbar</source>
      <translation>Pasek ikon rysowania</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="907"/>
      <source>Enable snap statusbar widget</source>
      <translation>Włącz widżet paska stanu przyborów przyciągania</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="910"/>
      <source>Draft snap widget</source>
      <translation>Widżet przyciągania rysowania</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="926"/>
      <source>Enable draft statusbar annotation scale widget</source>
      <translation>Włącz widżet skali dla adnotacji na pasku stanu</translation>
    </message>
    <message>
      <location filename="preferences-draftinterface.ui" line="929"/>
      <source>Annotation scale widget</source>
      <translation>Widżet adnotacji skali</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="567"/>
      <source>Draft Edit preferences</source>
      <translation>Ustawienia edycji rysunku roboczego</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="570"/>
      <source>Edit</source>
      <translation>Edycja</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="584"/>
      <source>Maximum number of contemporary edited objects</source>
      <translation>Liczba określająca ilość maksymalną aktualnie edytowanych obiektów</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="607"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Sets the maximum number of objects Draft Edit&lt;/p&gt;&lt;p&gt;can process at the same time&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Ustawia maksymalną liczbę obiektów Edycja Rysunek roboczy&lt;/p&gt;&lt;p&gt;jaką można przetwarzać w tym samym czasie&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="645"/>
      <source>Draft edit pick radius</source>
      <translation type="unfinished">Draft edit pick radius</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="668"/>
      <source>Controls pick radius of edit nodes</source>
      <translation>Kontroluje promień wyszukiwania edytowanych węzłów</translation>
    </message>
    <message>
      <location filename="preferences-dwg.ui" line="34"/>
      <source>Path to ODA file converter</source>
      <translation>Ścieżka do konwertera plików ODA</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="26"/>
      <source>This preferences dialog will be shown when importing/ exporting DXF files</source>
      <translation>Okno dialogowe będzie wyświetlane podczas importowania / eksportowania plików DXF</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="42"/>
      <source>Python importer is used, otherwise the newer C++ is used.
Note: C++ importer is faster, but is not as featureful yet</source>
      <translation>Jeśli opcja jest zaznaczona stosowany jest importer Python, w przeciwnym razie stosowany jest nowszy importer C++.
Uwaga: Importer C++ jest szybszy, ale nie jest jeszcze tak funkcjonalny</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="62"/>
      <source>Python exporter is used, otherwise the newer C++ is used.
Note: C++ importer is faster, but is not as featureful yet
      </source>
      <translation>Jeśli opcja jest zaznaczona stosowany jest eksporter Python, w przeciwnym razie stosowany jest nowszy eksporter C++.
Uwaga: eksporter C++ jest szybszy, ale nie jest jeszcze tak funkcjonalny
      </translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="88"/>
      <source>Allow FreeCAD to download the Python converter for DXF import and export.
You can also do this manually by installing the "dxf_library" workbench
from the Addon Manager.</source>
      <translation>Zezwól programowi FreeCAD na pobranie konwertera Python do importu i eksportu plików DXF.
Można to również zrobić ręcznie, instalując środowisko pracy „dxf_library”
z Menedżera dodatków.</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="139"/>
      <source>If unchecked, texts and mtexts won't be imported</source>
      <translation>Jeśli to pole nie jest zaznaczone, teksty / meta teksty nie zostaną zaimportowane</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="155"/>
      <source>If unchecked, points won't be imported</source>
      <translation>Jeśli to pole nie jest zaznaczone, punkty nie zostaną zaimportowane</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="171"/>
      <source>If checked, paper space objects will be imported too</source>
      <translation>Jeśli opcja jest zaznaczona, zaimportowane zostaną również obiekty z obszaru papieru</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="187"/>
      <source>If you want the non-named blocks (beginning with a *) to be imported too</source>
      <translation>Zaznacz ta opcje jeśli chcesz, aby zaimportować również bloki bez nazwy (zaczynające się od *)</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="214"/>
      <source>Only standard Part objects will be created (fastest)</source>
      <translation>Zostaną utworzone tylko standardowe obiekty części (opcja najszybsza)</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="233"/>
      <source>Parametric Draft objects will be created whenever possible</source>
      <translation>W miarę możliwości będą tworzone parametryczne rysunki obiektów</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="249"/>
      <source>Sketches will be created whenever possible</source>
      <translation>W miarę możliwości zostaną utworzone szkice</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="292"/>
      <source>Scale factor to apply to DXF files on import.
The factor is the conversion between the unit of your DXF file and millimeters.
Example: for files in millimeters: 1, in centimeters: 10,
                             in meters: 1000, in inches: 25.4, in feet: 304.8</source>
      <translation>Współczynnik skalowania do zastosowania przy imporcie plików DXF.
Współczynnik ten stanowi przelicznik pomiędzy jednostką z Twojego pliku DXF a milimetrami.
Przykład: dla plików w milimetrach: 1, w centymetrach: 10,
                             w metrach: 1000, w calach: 25,4, w stopach: 304.8</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="321"/>
      <source>Colors will be retrieved from the DXF objects whenever possible.
Otherwise default colors will be applied. </source>
      <translation>Barwy zostaną pobrane z obiektów DXF, gdy tylko będzie to możliwe.
W przeciwnym razie zostaną zastosowane barwy domyślne. </translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="342"/>
      <source>FreeCAD will try to join coincident objects into wires.
Note that this can take a while!</source>
      <translation>FreeCAD będzie próbował łączyć przypadkowe obiekty w linie łamane.
Zauważ, że to może być czasochłonne!</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="363"/>
      <source>Objects from the same layers will be joined into Draft Blocks,
turning the display faster, but making them less easily editable </source>
      <translation>Jeśli opcja jest zaznaczona, obiekty z tych samych warstw zostaną połączone w Bloki Szkiców,
umożliwi to szybsze obracanie obrazu na wyświetlaczu, ale czyni je mniej łatwymi do edycji </translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="384"/>
      <source>Imported texts will get the standard Draft Text size,
instead of the size they have in the DXF document</source>
      <translation>Importowane teksty uzyskają standardowy rozmiar tekstu w projekcie,
zamiast rozmiaru, który mają w dokumencie DXF</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="405"/>
      <source>If this is checked, DXF layers will be imported as Draft Layers</source>
      <translation>Jeśli opcja ta jest zaznaczona, warstwy DXF zostaną zaimportowane jako warstwy robocze</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="408"/>
      <source>Use Layers</source>
      <translation>Użyj warstw</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="428"/>
      <source>Hatches will be converted into simple wires</source>
      <translation>Kreskowanie zostanie zamienione na podstawowe linie</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="448"/>
      <source>If polylines have a width defined, they will be rendered
as closed wires with correct width</source>
      <translation>Jeśli polilinie mają zdefiniowaną szerokość, będą renderowane
jako zamknięte linie łamane o odpowiedniej szerokości</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="511"/>
      <source>Maximum length of each of the polyline segments.
If it is set to '0' the whole spline is treated as a straight segment.</source>
      <translation>Maksymalna długość każdego z segmentów polilinii.
Jeśli jest ustawiona na "0", cały splajn jest traktowany jako odcinek prosty.</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="550"/>
      <source>All objects containing faces will be exported as 3D polyfaces</source>
      <translation>Wszystkie obiekty zawierające ściany zostaną wyeksportowane jako trójwymiarowe powierzchnie czołowe</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="570"/>
      <source>Drawing Views will be exported as blocks.
This might fail for post DXF R12 templates.</source>
      <translation>Widoki rysunków roboczych będą eksportowane jako bloki.
Może się to nie udać w przypadku szablonów po DXF R12.</translation>
    </message>
    <message>
      <location filename="preferences-dxf.ui" line="594"/>
      <source>Exported objects will be projected to reflect the current view direction</source>
      <translation>Eksportowane obiekty będą rzutowane tak, aby odzwierciedlały aktualny kierunek widoku</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="47"/>
      <source>Method chosen for importing SVG object color to FreeCAD</source>
      <translation>Metoda wybrana do importu kolorów obiektów SVG do FreeCAD</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="82"/>
      <source>If checked, no units conversion will occur.
One unit in the SVG file will translate as one millimeter. </source>
      <translation>Jeśli opcja ta jest zaznaczona, nie będzie miała miejsca żadna konwersja jednostek.
Jedna jednostka z pliku SVG zostanie przeliczona na jeden milimetr. </translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="128"/>
      <source>Style of SVG file to write when exporting a sketch</source>
      <translation>Styl do zapisu pliku SVG podczas eksportu szkicu</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="158"/>
      <source>All white lines will appear in black in the SVG for better readability against white backgrounds</source>
      <translation>Wszystkie białe linie pojawią się w pliku SVG w kolorze czarnym, dla lepszej czytelności na białym tle</translation>
    </message>
    <message>
      <location filename="preferences-svg.ui" line="204"/>
      <source>Versions of Open CASCADE older than version 6.8 don't support arc projection.
In this case arcs will be discretized into small line segments.
This value is the maximum segment length. </source>
      <translation>Wersje Open CASCADE starsze niż wersja 6.8 nie obsługują rzutowania łuków.
W tym przypadku łuki zostaną rozbite na małe odcinki linii.
Wartość ta jest maksymalną długością segmentu. </translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="310"/>
      <source>If checked, an additional border is displayed around the grid, showing the main square size in the bottom left border</source>
      <translation>Jeżeli opcja jest zaznaczona, zostanie wyświetlona dodatkowa ramka wokół siatki, pokazująca główny rozmiar kwadratu w lewej dolnej części ramki</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="313"/>
      <source>Show grid border</source>
      <translation>Pokaż granicę siatki</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="475"/>
      <source>Override unit</source>
      <translation>Zastąp jednostkę</translation>
    </message>
    <message>
      <location filename="preferences-drafttexts.ui" line="495"/>
      <source>By leaving this field blank, the dimension measurements will be shown in the current unit defined in FreeCAD. By indicating a unit here such as m or cm, you can force new dimensions to be shown in that unit.</source>
      <translation>Jeśli to pole pozostanie puste, wartości wymiarów będą pokazane w bieżącej jednostce zdefiniowanej w FreeCAD. Wyznaczając jednostkę, taką jak m lub cm, można wymusić pokazanie nowych wymiarów w tej jednostce.</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="257"/>
      <source>The resolution to draw the patterns in. Default value is 128. Higher values give better resolutions, lower values make drawing faster</source>
      <translation>Rozdzielczość rysowania wzorów. Wartość domyślna to 128. Wyższe wartości dają lepszą rozdzielczość, niższe wartości umożliwiają szybsze rysowanie</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="280"/>
      <source>Hatch Pattern default size</source>
      <translation>Domyślny rozmiar wzoru kreskowania</translation>
    </message>
    <message>
      <location filename="preferences-draftvisual.ui" line="300"/>
      <source>The default size of hatch patterns</source>
      <translation>Domyślny rozmiar wzorów kreskowania</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="329"/>
      <source>If set, the grid will have its two main axes colored in red, green or blue when they match global axes</source>
      <translation>Jeśli opcja ta jest ustawiona, siatka będzie miała swoje dwie główne osie oznaczone kolorem czerwonym, zielonym lub niebieskim, jeśli pasują do osi globalnych</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="332"/>
      <source>Use colored axes</source>
      <translation>Użyj kolorowych osi</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="506"/>
      <source>Grid color and transparency</source>
      <translation>Kolor i przezroczystość siatki</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="526"/>
      <source>The color of the grid</source>
      <translation>Kolor siatki</translation>
    </message>
    <message>
      <location filename="preferences-draftsnap.ui" line="546"/>
      <source>The overall transparency of the grid</source>
      <translation>Ogólna przezroczystość siatki</translation>
    </message>
  </context>
  <context>
    <name>ImportDWG</name>
    <message>
      <location filename="importDWG.py" line="276"/>
      <source>Converting: </source>
      <translation>Konwertowanie: </translation>
    </message>
    <message>
      <location filename="importDWG.py" line="232"/>
      <source>Conversion successful</source>
      <translation>Konwersja zakończona</translation>
    </message>
  </context>
  <context>
    <name>ImportSVG</name>
    <message>
      <location filename="importSVG.py" line="1808"/>
      <source>Unknown SVG export style, switching to Translated</source>
      <translation type="unfinished">Unknown SVG export style, switching to Translated</translation>
    </message>
    <message>
      <location filename="importSVG.py" line="1828"/>
      <source>The export list contains no object with a valid bounding box</source>
      <translation>Lista eksportowa nie zawiera żadnego obiektu w aktualnym polu wyboru</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="gui_snapper.py" line="1468"/>
      <source>Draft Snap</source>
      <translation>Przyciągnij projekt</translation>
    </message>
  </context>
  <context>
    <name>draft</name>
    <message>
      <location filename="DraftGui.py" line="233"/>
      <source>Draft Command Bar</source>
      <translation>Pasek komend projektu</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="698"/>
      <source>Toggle construction mode</source>
      <translation>Przełącz tryb konstrukcyjny</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1894"/>
      <source>Autogroup off</source>
      <translation>Wyłącz automatyczne grupowanie</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="610"/>
      <source>active command:</source>
      <translation>aktywne polecenie:</translation>
    </message>
    <message>
      <location filename="gui_groups.py" line="276"/>
      <source>None</source>
      <translation>Brak</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="612"/>
      <source>Active Draft command</source>
      <translation>Aktywne polecenie kreślarskie</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="613"/>
      <source>X coordinate of next point</source>
      <translation>Współrzędna X następnego punktu</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="930"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="615"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="616"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="617"/>
      <source>Y coordinate of next point</source>
      <translation>Współrzędna Y następnego punktu</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="618"/>
      <source>Z coordinate of next point</source>
      <translation>Współrzędna Z następnego punktu</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="619"/>
      <source>Enter point</source>
      <translation>Wprowadź punkt</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="620"/>
      <source>Enter a new point with the given coordinates</source>
      <translation>Wprowadź nowy punkt z koordynatami</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="621"/>
      <source>Length</source>
      <translation>Długość</translation>
    </message>
    <message>
      <location filename="gui_trimex.py" line="334"/>
      <source>Angle</source>
      <translation>Kąt</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="623"/>
      <source>Length of current segment</source>
      <translation>Długość odcinka bieżącego</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="624"/>
      <source>Angle of current segment</source>
      <translation>Kąt bieżącego segmentu</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="980"/>
      <source>Radius</source>
      <translation>Promień</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="981"/>
      <source>Radius of Circle</source>
      <translation>Promień okręgu</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="635"/>
      <source>If checked, command will not finish until you press the command button again</source>
      <translation>Jeżeli opcja jest zaznaczona, polecenie nie zostanie zakończone do momentu naciśnięcia przycisku polecenia ponownie</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="637"/>
      <source>If checked, an OCC-style offset will be performed instead of the classic offset</source>
      <translation>Jeśli zaznaczone, będzie wykonywane przesunięcie w stylu OCC zamiast classic</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="638"/>
      <source>&amp;OCC-style offset</source>
      <translation>&amp;OCC-styl przesunięcie</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="658"/>
      <source>Sides</source>
      <translation>Boki</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="659"/>
      <source>Number of sides</source>
      <translation>Liczba boków</translation>
    </message>
    <message>
      <location filename="gui_offset.py" line="321"/>
      <source>Offset</source>
      <translation>Odsunięcie</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="552"/>
      <source>Auto</source>
      <translation>Automatycznie</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="665"/>
      <source>Text string to draw</source>
      <translation>Krzywe tekstu do rysowania</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="666"/>
      <source>String</source>
      <translation>Ciąg</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="667"/>
      <source>Height of text</source>
      <translation>Wysokość tekstu</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="668"/>
      <source>Height</source>
      <translation>Wysokość</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="669"/>
      <source>Intercharacter spacing</source>
      <translation>Odstęp pomiędzy znakami</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="670"/>
      <source>Tracking</source>
      <translation>Śledzenie</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="671"/>
      <source>Full path to font file:</source>
      <translation>Pełna ścieżka do pliku czcionki:</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="672"/>
      <source>Open a FileChooser for font file</source>
      <translation>Otwórz okno wyboru pliku dla czcionki</translation>
    </message>
    <message>
      <location filename="gui_lines.py" line="79"/>
      <source>Line</source>
      <translation>Linia</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="828"/>
      <source>DWire</source>
      <translation>DWire</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="845"/>
      <source>Circle</source>
      <translation>Okrąg</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="861"/>
      <source>Center X</source>
      <translation>Środek X</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="72"/>
      <source>Arc</source>
      <translation>Łuk</translation>
    </message>
    <message>
      <location filename="gui_points.py" line="67"/>
      <source>Point</source>
      <translation>Punkt</translation>
    </message>
    <message>
      <location filename="gui_labels.py" line="70"/>
      <source>Label</source>
      <translation>Etykieta</translation>
    </message>
    <message>
      <location filename="gui_trimex.py" line="318"/>
      <source>Distance</source>
      <translation>Odległość</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="969"/>
      <source>Trim</source>
      <translation>Przytnij</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1067"/>
      <source>Pick Object</source>
      <translation>Wybierz obiekt</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1073"/>
      <source>Edit</source>
      <translation>Edycja</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1124"/>
      <source>Global X</source>
      <translation>Globalna X</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1125"/>
      <source>Global Y</source>
      <translation>Globalna Y</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1126"/>
      <source>Global Z</source>
      <translation>Globalna Z</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1129"/>
      <source>Local X</source>
      <translation>Lokalna X</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1130"/>
      <source>Local Y</source>
      <translation>Lokalna Y</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1131"/>
      <source>Local Z</source>
      <translation>Lokalna Z</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1360"/>
      <source>Invalid Size value. Using 200.0.</source>
      <translation>Nieprawidłowa wartość rozmiaru. Użyj 200.0.</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1368"/>
      <source>Invalid Tracking value. Using 0.</source>
      <translation>Nieprawidłowa wartość śledzenia. Użyj 0.</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1382"/>
      <source>Please enter a text string.</source>
      <translation>Wprowadź tekst.</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1391"/>
      <source>Select a Font file</source>
      <translation>Wybierz plik czcionki</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1421"/>
      <source>Please enter a font file.</source>
      <translation>Wprowadź plik czcionki.</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="1887"/>
      <source>Autogroup: </source>
      <translation>Autogrupowanie: </translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="2188"/>
      <source>Faces</source>
      <translation>Ściany</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="2189"/>
      <source>Remove</source>
      <translation>Skasuj</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="2190"/>
      <source>Add</source>
      <translation>Dodaj</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="2191"/>
      <source>Facebinder elements</source>
      <translation>Elementy facebindera</translation>
    </message>
    <message>
      <location filename="gui_splines.py" line="68"/>
      <source>BSpline</source>
      <translation>BSpline</translation>
    </message>
    <message>
      <location filename="gui_beziers.py" line="72"/>
      <source>BezCurve</source>
      <translation>Krzywa Beziera</translation>
    </message>
    <message>
      <location filename="gui_rotate.py" line="277"/>
      <source>Copy</source>
      <translation>Kopiuj</translation>
    </message>
    <message>
      <location filename="importDXF.py" line="146"/>
      <source>The DXF import/export libraries needed by FreeCAD to handle
the DXF format were not found on this system.
Please either enable FreeCAD to download these libraries:
  1 - Load Draft workbench
  2 - Menu Edit &gt; Preferences &gt; Import-Export &gt; DXF &gt; Enable downloads
Or download these libraries manually, as explained on
https://github.com/yorikvanhavre/Draft-dxf-importer
To enabled FreeCAD to download these libraries, answer Yes.</source>
      <translation>Biblioteki importu/eksportu DXF potrzebne FreeCAD do obługi formatu DXF nie zotały znalezione w systemie.
Proszę albo pozwolić FreeCAD na pobranie tych bibliotek:
 1 - Załaduj Środowisko pracy Draft
 2 - Menu Edycja &gt; Ustawienia &gt; Import-Eksport &gt; DXF &gt; Włącz pobieranie
Lub pobierz te biblioteki ręcznie, jak wyjaśniono na 
https://github.com/yorikvanhavre/Draft-dxf-importer
żeby pozwolić FreeCAD na pobranie bibliotek, odpowiedz tak.</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="629"/>
      <source>Relative</source>
      <translation>Względny</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="636"/>
      <source>Continue</source>
      <translation>Kontynuuj</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="650"/>
      <source>Close</source>
      <translation>Zamknij</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="60"/>
      <source>Fill</source>
      <translation>Wypełnij</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="61"/>
      <source>Exit</source>
      <translation>Wyjdź</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="62"/>
      <source>Snap On/Off</source>
      <translation>Przyciąganie włączone/Wyłączone</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="63"/>
      <source>Increase snap radius</source>
      <translation>Zwiększ promień przyciągania</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="64"/>
      <source>Decrease snap radius</source>
      <translation>Zmniejsz promień przyciągania</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="65"/>
      <source>Restrict X</source>
      <translation>Ogranicz X</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="66"/>
      <source>Restrict Y</source>
      <translation>Ogranicz Y</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="67"/>
      <source>Restrict Z</source>
      <translation>Ogranicz Z</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="656"/>
      <source>Select edge</source>
      <translation>Wybierz krawędź</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="69"/>
      <source>Add custom snap point</source>
      <translation>Dodaj niestandardowy punkt przyciągania</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="70"/>
      <source>Length mode</source>
      <translation>Tryb długości</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="652"/>
      <source>Wipe</source>
      <translation>Wyczyść</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="72"/>
      <source>Set Working Plane</source>
      <translation>Ustaw Płaszczyznę Roboczą</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="73"/>
      <source>Cycle snap object</source>
      <translation type="unfinished">Cycle snap object</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="626"/>
      <source>Check this to lock the current angle</source>
      <translation>Zaznacz to, aby zablokować bieżący kąt</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="630"/>
      <source>Coordinates relative to last point or absolute</source>
      <translation>Współrzędne w stosunku do ostatniego punktu lub bezwzględne</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="631"/>
      <source>Filled</source>
      <translation>Wypełniony</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="633"/>
      <source>Finish</source>
      <translation>Zakończ</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="634"/>
      <source>Finishes the current drawing or editing operation</source>
      <translation>Kończy bieżącą operację rysowania lub edycji</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="648"/>
      <source>&amp;Undo (CTRL+Z)</source>
      <translation>&amp;Cofnij (CTRL+Z)</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="649"/>
      <source>Undo the last segment</source>
      <translation>Cofnij ostatni segment</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="651"/>
      <source>Finishes and closes the current line</source>
      <translation>Kończy i zamyka bieżący wiersz</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="653"/>
      <source>Wipes the existing segments of this line and starts again from the last point</source>
      <translation>Czyści istniejące segmenty tego wierszu i rozpoczyna ponownie od ostatniego punktu</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="654"/>
      <source>Set WP</source>
      <translation>Ustaw WP</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="655"/>
      <source>Reorients the working plane on the last segment</source>
      <translation>Zmienia orientację płaszczyzny roboczej na ostatnim segmencie</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="657"/>
      <source>Selects an existing edge to be measured by this dimension</source>
      <translation>Wybiera istniejącą krawędź w celu pomiaru przez ten wymiar</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="662"/>
      <source>If checked, objects will be copied instead of moved. Preferences -&gt; Draft -&gt; Global copy mode to keep this mode in next commands</source>
      <translation>Jeśli opcja jest zaznaczona, obiekty będą kopiowane, a nie przenoszone. Preferencje -&gt; Wersja robocza -&gt; Globalny tryb kopiowania, opcja umożliwia zachowanie tego trybu w następnych poleceniach</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="59"/>
      <source>Subelement mode</source>
      <translation>Tryb podelementów</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="663"/>
      <source>Modify subelements</source>
      <translation>Modyfikuj podelementy</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="664"/>
      <source>If checked, subelements will be modified instead of entire objects</source>
      <translation type="unfinished">If checked, subelements will be modified instead of entire objects</translation>
    </message>
    <message>
      <location filename="gui_beziers.py" line="251"/>
      <source>CubicBezCurve</source>
      <translation>PrzestrzenieBezKrzywych</translation>
    </message>
    <message>
      <location filename="gui_selectplane.py" line="363"/>
      <source>Top</source>
      <translation>Od góry</translation>
    </message>
    <message>
      <location filename="gui_selectplane.py" line="375"/>
      <source>Front</source>
      <translation>Od przodu</translation>
    </message>
    <message>
      <location filename="gui_selectplane.py" line="387"/>
      <source>Side</source>
      <translation>Strona</translation>
    </message>
    <message>
      <location filename="gui_selectplane.py" line="530"/>
      <source>Current working plane</source>
      <translation>Bieżąca płaszczyzna robocza</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="632"/>
      <source>Check this if the object should appear as filled, otherwise it will appear as wireframe. Not available if Draft preference option 'Use Part Primitives' is enabled</source>
      <translation type="unfinished">Check this if the object should appear as filled, otherwise it will appear as wireframe. Not available if Draft preference option 'Use Part Primitives' is enabled</translation>
    </message>
    <message>
      <location filename="gui_lines.py" line="364"/>
      <source>Polyline</source>
      <translation>Linia łamana</translation>
    </message>
    <message>
      <location filename="InitGui.py" line="46"/>
      <source>Draft</source>
      <translation>Szkic</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="74"/>
      <source>Toggle near snap on/off</source>
      <translation>Przełącz w pobliżu miejsca przyciągania włącz/wyłącz</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="673"/>
      <source>Create text</source>
      <translation>Utwórz tekst</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="674"/>
      <source>Press this button to create the text object, or finish your text with two blank lines</source>
      <translation>Naciśnij ten przycisk, aby utworzyć obiekt tekstowy, lub zakończ tekst dwiema pustymi liniami</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="862"/>
      <source>Center Y</source>
      <translation>Wyśrodkuj Y</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="863"/>
      <source>Center Z</source>
      <translation>Wyśrodkuj Z</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="914"/>
      <source>Offset distance</source>
      <translation>Odległość przesunięcia</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="973"/>
      <source>Trim distance</source>
      <translation>Odległość przycięcia</translation>
    </message>
    <message>
      <location filename="InitGui.py" line="159"/>
      <source>Wire</source>
      <translation>Linia</translation>
    </message>
    <message>
      <location filename="DraftGui.py" line="697"/>
      <source>Change default style for new objects</source>
      <translation>Zmień domyślny styl dla nowych obiektów</translation>
    </message>
    <message>
      <location filename="make_label.py" line="194"/>
      <source>No active document. Aborting.</source>
      <translation>Brak aktywnego dokumentu. Przerwanie.</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="151"/>
      <source>Object must be a closed shape</source>
      <translation>Obiekt musi być zamkniętym kształtem</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="153"/>
      <source>No solid object created</source>
      <translation>Nie utworzono bryły</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="276"/>
      <source>Faces must be coplanar to be refined</source>
      <translation>Ściany muszą być współpłaszczyznowe, aby można je było poprawić</translation>
    </message>
    <message>
      <location filename="downgrade.py" line="230"/>
      <source>Upgrade: Unknown force method:</source>
      <translation type="unfinished">Upgrade: Unknown force method:</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="450"/>
      <source>Found groups: closing each open object inside</source>
      <translation>Znaleziono grupy: zamykanie każdego otwartego obiektu wewnątrz</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="456"/>
      <source>Found meshes: turning into Part shapes</source>
      <translation type="unfinished">Found meshes: turning into Part shapes</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="464"/>
      <source>Found 1 solidifiable object: solidifying it</source>
      <translation type="unfinished">Found 1 solidifiable object: solidifying it</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="469"/>
      <source>Found 2 objects: fusing them</source>
      <translation>Znaleziono 2 obiekty: łączenie ich</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="480"/>
      <source>Found object with several coplanar faces: refine them</source>
      <translation>Znaleziono obiekt z kilkoma współbieżnymi powierzchniami: dopracuj je</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="486"/>
      <source>Found 1 non-parametric objects: draftifying it</source>
      <translation>Znaleziono 1 nieparametryczny obiekt: szkicowanie go</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="497"/>
      <source>Found 1 closed sketch object: creating a face from it</source>
      <translation>Znaleziono 1 zamknięty obiekt rysunku: tworzenie z niego powierzchni</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="502"/>
      <source>Found closed wires: creating faces</source>
      <translation>Znaleziono zamknięte linie łamane: tworzę płaszczyznę</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="507"/>
      <source>Found several wires or edges: wiring them</source>
      <translation>Znaleziono kilka linii łamanych lub krawędzi: łączenie ich</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="512"/>
      <source>trying: closing it</source>
      <translation type="unfinished">trying: closing it</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="514"/>
      <source>Found 1 open wire: closing it</source>
      <translation>Znaleziono 1 otwarty przewod: zamknięcie go</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="531"/>
      <source>Found 1 object: draftifying it</source>
      <translation>Znaleziono 1 obiekt: redakcja jego</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="536"/>
      <source>Found points: creating compound</source>
      <translation>Znalezione punkty: tworzenie złożenia</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="541"/>
      <source>Found several non-treatable objects: creating compound</source>
      <translation type="unfinished">Found several non-treatable objects: creating compound</translation>
    </message>
    <message>
      <location filename="upgrade.py" line="544"/>
      <source>Unable to upgrade these objects.</source>
      <translation>Nie można uaktualnić tych obiektów.</translation>
    </message>
    <message>
      <location filename="mirror.py" line="90"/>
      <source>No object given</source>
      <translation>Nie podano obiektu</translation>
    </message>
    <message>
      <location filename="mirror.py" line="94"/>
      <source>The two points are coincident</source>
      <translation>Te dwa punkty są zbieżne</translation>
    </message>
    <message>
      <location filename="mirror.py" line="114"/>
      <source>mirrored)</source>
      <translation>odbicie lustrzane)</translation>
    </message>
    <message>
      <location filename="downgrade.py" line="238"/>
      <source>Found 1 block: exploding it</source>
      <translation>Znaleziono 1 blok: rozbijanie go</translation>
    </message>
    <message>
      <location filename="downgrade.py" line="246"/>
      <source>Found 1 multi-solids compound: exploding it</source>
      <translation>Znaleziono 1 złożenie wielu brył: rozstrzeliwuję to</translation>
    </message>
    <message>
      <location filename="downgrade.py" line="253"/>
      <source>Found 1 parametric object: breaking its dependencies</source>
      <translation>Znaleziono 1 obiekt parametryczny: łamanie jego zależności</translation>
    </message>
    <message>
      <location filename="downgrade.py" line="261"/>
      <source>Found 2 objects: subtracting them</source>
      <translation>Znaleziono 2 obiekty: odejmowanie ich</translation>
    </message>
    <message>
      <location filename="downgrade.py" line="268"/>
      <source>Found several faces: splitting them</source>
      <translation>Znaleziono kilka powierzchni: dzielenie ich</translation>
    </message>
    <message>
      <location filename="downgrade.py" line="273"/>
      <source>Found several objects: subtracting them from the first one</source>
      <translation>Znaleziono kilka obiektów: odejmowanie ich od pierwszego</translation>
    </message>
    <message>
      <location filename="downgrade.py" line="278"/>
      <source>Found 1 face: extracting its wires</source>
      <translation>Znaleziono 1 powierzchnię: wyodrębnianie jej linii</translation>
    </message>
    <message>
      <location filename="downgrade.py" line="284"/>
      <source>Found only wires: extracting their edges</source>
      <translation type="unfinished">Found only wires: extracting their edges</translation>
    </message>
    <message>
      <location filename="downgrade.py" line="288"/>
      <source>No more downgrade possible</source>
      <translation type="unfinished">No more downgrade possible</translation>
    </message>
    <message>
      <location filename="make_label.py" line="235"/>
      <source>Wrong input: object not in document.</source>
      <translation>Nieprawidłowe dane wejściowe: obiektu nie ma w dokumencie.</translation>
    </message>
    <message>
      <location filename="make_pointarray.py" line="135"/>
      <source>Wrong input: point object doesn't have 'Geometry', 'Links', or 'Components'.</source>
      <translation>Nieprawidłowe dane wejściowe: obiekt punktowy nie posiada 'Geometrii', 'Powiązań' lub 'Komponentów'.</translation>
    </message>
    <message>
      <location filename="make_label.py" line="214"/>
      <source>Wrong input: must be a placement, a vector, or a rotation.</source>
      <translation>Nieprawidłowe dane wejściowe: musi być umiejscowieniem, wektorem lub obrotem.</translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="130"/>
      <source>Points: </source>
      <translation>Punkty: </translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="131"/>
      <source>Wrong input: must be list or tuple of three points exactly.</source>
      <translation>Błędne dane wejściowe: musi to być lista lub tupla składające się dokładnie z trzech punktów.</translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="138"/>
      <source>Placement: </source>
      <translation>Umiejscowienie: </translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="139"/>
      <source>Wrong input: incorrect type of placement.</source>
      <translation>Nieprawidłowe dane wejściowe: nieprawidłowy typ umiejscowienia.</translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="153"/>
      <source>Wrong input: incorrect type of points.</source>
      <translation>Nieprawidłowe dane wejściowe: nieprawidłowy typ punktów.</translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="159"/>
      <source>Cannot generate shape: </source>
      <translation>Nie można wygenerować kształtu: </translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="166"/>
      <source>Radius:</source>
      <translation>Promień:</translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="167"/>
      <source>Center:</source>
      <translation>Środek:</translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="170"/>
      <source>Create primitive object</source>
      <translation>Utwórz obiekt bryły pierwotnej</translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="204"/>
      <source>Final placement:</source>
      <translation>Ostateczne umiejscowienie:</translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="195"/>
      <source>Face: True</source>
      <translation>Powierzchnia: Prawda</translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="197"/>
      <source>Support:</source>
      <translation>Wsparcie techniczne:</translation>
    </message>
    <message>
      <location filename="make_arc_3points.py" line="198"/>
      <source>Map mode:</source>
      <translation>Tryb mapy:</translation>
    </message>
    <message>
      <location filename="make_fillet.py" line="145"/>
      <source>length:</source>
      <translation>długość:</translation>
    </message>
    <message>
      <location filename="make_fillet.py" line="132"/>
      <source>Two elements are needed.</source>
      <translation>Potrzebne są dwa elementy.</translation>
    </message>
    <message>
      <location filename="make_fillet.py" line="139"/>
      <source>Radius is too large</source>
      <translation>Promień jest zbyt duży</translation>
    </message>
    <message>
      <location filename="make_fillet.py" line="145"/>
      <source>Segment</source>
      <translation>Segment</translation>
    </message>
    <message>
      <location filename="make_fillet.py" line="165"/>
      <source>Removed original objects.</source>
      <translation>Usunięto oryginalne obiekty.</translation>
    </message>
    <message>
      <location filename="make_text.py" line="101"/>
      <source>Wrong input: must be a list of strings or a single string.</source>
      <translation>Nieprawidłowe dane wejściowe: musi być listą ciągów znaków lub pojedynczym ciągiem znaków.</translation>
    </message>
    <message>
      <location filename="task_circulararray.py" line="491"/>
      <source>Circular array</source>
      <translation>Szyk kołowy</translation>
    </message>
    <message>
      <location filename="make_circulararray.py" line="144"/>
      <source>Wrong input: must be a number or quantity.</source>
      <translation>Nieprawidłowe dane wejściowe: musi być liczbą lub ilością.</translation>
    </message>
    <message>
      <location filename="make_orthoarray.py" line="119"/>
      <source>Wrong input: must be an integer number.</source>
      <translation>Nieprawidłowe dane wejściowe: musi być liczbą całkowitą.</translation>
    </message>
    <message>
      <location filename="make_label.py" line="203"/>
      <source>Wrong input: must be a vector.</source>
      <translation>Błędne wejście: musi być wektorem.</translation>
    </message>
    <message>
      <location filename="task_polararray.py" line="438"/>
      <source>Polar array</source>
      <translation>Szyk kołowy</translation>
    </message>
    <message>
      <location filename="make_label.py" line="307"/>
      <source>Wrong input: must be a number.</source>
      <translation>Błędne wejście: musi być liczbą.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="173"/>
      <source>This function is deprecated. Do not use this function directly.</source>
      <translation>Ta funkcja jest przestarzała. Nie używaj tej funkcji bezpośrednio.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="174"/>
      <source>Use one of 'make_linear_dimension', or 'make_linear_dimension_obj'.</source>
      <translation>Użyj jednego z 'utwórz linię wymiarową', lub 'utwórz obiekt linia wymiarowa'.</translation>
    </message>
    <message>
      <location filename="make_label.py" line="229"/>
      <source>Wrong input: object must not be a list.</source>
      <translation>Nieprawidłowe dane wejściowe: obiekt nie może być listą.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="452"/>
      <source>Wrong input: object doesn't have a 'Shape' to measure.</source>
      <translation>Nieprawidłowe dane wejściowe: obiekt nie ma 'Kształtu' do pomiaru.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="331"/>
      <source>Wrong input: object doesn't have at least one element in 'Vertexes' to use for measuring.</source>
      <translation>Nieprawidłowe dane wejściowe: obiekt nie posiada co najmniej jednego elementu 'Wierzchołek', który mógłby zostać użyty do pomiaru.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="463"/>
      <source>Wrong input: must be an integer.</source>
      <translation>Błędne wprowadzenie: musi być liczbą całkowitą.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="343"/>
      <source>i1: values below 1 are not allowed; will be set to 1.</source>
      <translation>i1: wartości poniżej 1 nie są dozwolone; zostaną ustawione na 1.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="363"/>
      <source>Wrong input: vertex not in object.</source>
      <translation>Nieprawidłowe wejście: wierzchołek nie jest w obiekcie.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="359"/>
      <source>i2: values below 1 are not allowed; will be set to the last vertex in the object.</source>
      <translation>i2: wartości poniżej 1 nie są dozwolone; zostaną ustawione na ostatni wierzchołek w obiekcie.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="456"/>
      <source>Wrong input: object doesn't have at least one element in 'Edges' to use for measuring.</source>
      <translation>Nieprawidłowe dane wejściowe: obiekt nie posiada co najmniej jednego elementu 'krawędź' do pomiaru.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="468"/>
      <source>index: values below 1 are not allowed; will be set to 1.</source>
      <translation>indeks: wartości poniżej 1 nie są dozwolone; zostaną ustawione na 1.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="472"/>
      <source>Wrong input: index doesn't correspond to an edge in the object.</source>
      <translation>Nieprawidłowe dane wejściowe: indeks nie odpowiada krawędzi w obiekcie.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="476"/>
      <source>Wrong input: index doesn't correspond to a circular edge.</source>
      <translation>Nieprawidłowe dane wejściowe: obiekt nie odpowiada krawędzi kolistej.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="487"/>
      <source>Wrong input: must be a string, 'radius' or 'diameter'.</source>
      <translation>Nieprawidłowe dane wejściowe: musi być ciągiem znaków, 'promień' lub 'średnica'.</translation>
    </message>
    <message>
      <location filename="make_dimension.py" line="582"/>
      <source>Wrong input: must be a list with two angles.</source>
      <translation>Nieprawidłowe dane wejściowe: musi być listą zawierającą dwa kąty.</translation>
    </message>
    <message>
      <location filename="make_layer.py" line="69"/>
      <source>Layers</source>
      <translation>Warstwy</translation>
    </message>
    <message>
      <location filename="gui_layers.py" line="47"/>
      <source>Layer</source>
      <translation>Warstwa</translation>
    </message>
    <message>
      <location filename="make_layer.py" line="157"/>
      <source>Wrong input: it must be a string.</source>
      <translation>Nieprawidłowe dane wejściowe: musi być ciągiem znaków.</translation>
    </message>
    <message>
      <location filename="make_layer.py" line="188"/>
      <source>Wrong input: must be a tuple of three floats 0.0 to 1.0.</source>
      <translation>Musi być tuplom trzech liczb zmiennoprzecinkowych od 0.0 do 1.0.</translation>
    </message>
    <message>
      <location filename="make_layer.py" line="212"/>
      <source>Wrong input: must be 'Solid', 'Dashed', 'Dotted', or 'Dashdot'.</source>
      <translation>Nieprawidłowe dane wejściowe: musi to być 'ciągła', 'kreskowana', 'kropkowana' lub 'kreska kropka'.</translation>
    </message>
    <message>
      <location filename="make_layer.py" line="220"/>
      <source>Wrong input: must be a number between 0 and 100.</source>
      <translation>Błędne dane wejściowe: musi być liczbą z zakresu 0 a 100.</translation>
    </message>
    <message>
      <location filename="make_label.py" line="250"/>
      <source>Wrong input: must be a list or tuple of strings, or a single string.</source>
      <translation>Nieprawidłowe dane wejściowe: musi to być lista lub tupla ciągów znaków, albo pojedynczy ciąg znaków.</translation>
    </message>
    <message>
      <location filename="make_patharray.py" line="246"/>
      <source>Wrong input: must be 'Original', 'Frenet', or 'Tangent'.</source>
      <translation>Nieprawidłowe dane wejściowe: musi to być 'oryginalny', 'swobodny' lub 'styczny'.</translation>
    </message>
    <message>
      <location filename="make_sketch.py" line="103"/>
      <source>No shape found
</source>
      <translation>Nie znaleziono kształtu
</translation>
    </message>
    <message>
      <location filename="make_sketch.py" line="109"/>
      <source>All Shapes must be planar
</source>
      <translation>Wszystkie kształty muszą być płaskie
</translation>
    </message>
    <message>
      <location filename="make_sketch.py" line="135"/>
      <source>All Shapes must be coplanar
</source>
      <translation>Wszystkie kształty muszą być współpłaszczyznowe
</translation>
    </message>
    <message>
      <location filename="make_orthoarray.py" line="60"/>
      <source>Internal orthogonal array</source>
      <translation>Wewnętrzna tablica ortogonalna</translation>
    </message>
    <message>
      <location filename="make_orthoarray.py" line="87"/>
      <source>Wrong input: must be a number or vector.</source>
      <translation>Błędne dane wejściowe: musi być liczbą lub wektorem.</translation>
    </message>
    <message>
      <location filename="make_orthoarray.py" line="98"/>
      <source>Input: single value expanded to vector.</source>
      <translation>Wpisz: pojedyncza wartość rozszerzona do wektora.</translation>
    </message>
    <message>
      <location filename="make_orthoarray.py" line="129"/>
      <source>Input: number of elements must be at least 1. It is set to 1.</source>
      <translation>Wpisz: liczba elementów musi wynosić co najmniej 1. Jest ustawiona na 1.</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="400"/>
      <source>Orthogonal array</source>
      <translation>Szyk prostopadły</translation>
    </message>
    <message>
      <location filename="make_orthoarray.py" line="350"/>
      <source>Orthogonal array 2D</source>
      <translation>Tablica ortogonalna 2D</translation>
    </message>
    <message>
      <location filename="make_orthoarray.py" line="424"/>
      <source>Rectangular array</source>
      <translation>Szyk prostokątny</translation>
    </message>
    <message>
      <location filename="make_orthoarray.py" line="501"/>
      <source>Rectangular array 2D</source>
      <translation>Szyk prostokątny 2D</translation>
    </message>
    <message>
      <location filename="make_label.py" line="262"/>
      <source>Wrong input: subelement not in object.</source>
      <translation>Nieprawidłowe dane wejściowe: element podrzędny nie należy do obiektu.</translation>
    </message>
    <message>
      <location filename="make_label.py" line="276"/>
      <source>Wrong input: must be a string, 'Custom', 'Name', 'Label', 'Position', 'Length', 'Area', 'Volume', 'Tag', or 'Material'.</source>
      <translation>Nieprawidłowe dane wejściowe: musi być ciągiem znaków, 'Niestandardowy', 'Nazwa', 'Etykieta', 'Pozycja', 'Długość', 'Obszar', 'Objętość', 'Tag', lub 'Material'.</translation>
    </message>
    <message>
      <location filename="make_label.py" line="285"/>
      <source>Wrong input: must be a string.</source>
      <translation>Nieprawidłowe dane wejściowe: musi być ciągiem znaków.</translation>
    </message>
    <message>
      <location filename="make_label.py" line="298"/>
      <source>Wrong input: must be a string, 'Horizontal', 'Vertical', or 'Custom'.</source>
      <translation>Nieprawidłowe dane wejściowe: musi być ciągiem znaków 'Poziomy', 'Pionowy' lub 'Niestandardowy'.</translation>
    </message>
    <message>
      <location filename="make_label.py" line="314"/>
      <source>Wrong input: must be a list of at least two vectors.</source>
      <translation>Nieprawidłowe dane wejściowe: musi być listą co najmniej dwóch wektorów.</translation>
    </message>
    <message>
      <location filename="make_label.py" line="347"/>
      <source>Direction is not 'Custom'; points won't be used.</source>
      <translation>Kierunek nie jest 'Niestandadowy'; punkty nie będą używane.</translation>
    </message>
    <message>
      <location filename="make_label.py" line="374"/>
      <source>Wrong input: must be a list of two elements. For example, [object, 'Edge1'].</source>
      <translation>Błędne dane wejściowe: musi być listą dwóch elementów. Na przykład [obiekt, 'Krawędź1'].</translation>
    </message>
    <message>
      <location filename="shapestring.py" line="73"/>
      <source>ShapeString: string has no wires</source>
      <translation>ShapeString: ciąg nie ma linii łamanych</translation>
    </message>
    <message>
      <location filename="pointarray.py" line="160"/>
      <source>added property 'ExtraPlacement'</source>
      <translation>dodano właściwość 'Ekstralokacja'</translation>
    </message>
    <message>
      <location filename="patharray.py" line="284"/>
      <source>, path object doesn't have 'Edges'.</source>
      <translation>, obiekt ścieżki nie ma 'Krawędzi'.</translation>
    </message>
    <message>
      <location filename="patharray.py" line="407"/>
      <source>'PathObj' property will be migrated to 'PathObject'</source>
      <translation>Właściwość 'PathObj' zostanie przeniesiona do 'PathObject'</translation>
    </message>
    <message>
      <location filename="patharray.py" line="525"/>
      <source>Cannot calculate path tangent. Copy not aligned.</source>
      <translation>Nie można obliczyć stycznej ścieżki. Kopia nie zostanie wyrównana.</translation>
    </message>
    <message>
      <location filename="patharray.py" line="541"/>
      <source>Tangent and normal are parallel. Copy not aligned.</source>
      <translation>Styczna i normalna są równoległe. Kopiowanie nie jest wyrównywane.</translation>
    </message>
    <message>
      <location filename="patharray.py" line="558"/>
      <source>Cannot calculate path normal, using default.</source>
      <translation>Nie można obliczyć ścieżki normalnej, zastosowano domyślną.</translation>
    </message>
    <message>
      <location filename="patharray.py" line="565"/>
      <source>Cannot calculate path binormal. Copy not aligned.</source>
      <translation>Nie można obliczyć ścieżki dwuwymiarowej. Kopia nie została wyrównana.</translation>
    </message>
    <message>
      <location filename="patharray.py" line="571"/>
      <source>AlignMode {} is not implemented</source>
      <translation>Wyrównanie {} nie jest zaimplementowane</translation>
    </message>
    <message>
      <location filename="draft_annotation.py" line="105"/>
      <source>added view property 'ScaleMultiplier'</source>
      <translation>dodano właściwość widoku 'ScaleMultiplier'</translation>
    </message>
    <message>
      <location filename="draft_annotation.py" line="130"/>
      <source>migrated 'DraftText' type to 'Text'</source>
      <translation>zmigrowano typ 'Tekst rysunkowy' na 'Tekst'</translation>
    </message>
    <message>
      <location filename="view_layer.py" line="358"/>
      <source>Activate this layer</source>
      <translation>Uaktywnij tę warstwę</translation>
    </message>
    <message>
      <location filename="view_layer.py" line="364"/>
      <source>Select layer contents</source>
      <translation>Wybierz zawartość warstwy</translation>
    </message>
    <message>
      <location filename="init_draft_statusbar.py" line="147"/>
      <source>custom</source>
      <translation type="unfinished">custom</translation>
    </message>
    <message>
      <location filename="init_draft_statusbar.py" line="133"/>
      <source>Unable to convert input into a scale factor</source>
      <translation>Nie można przekonwertować danych wejściowych do współczynnika skali</translation>
    </message>
    <message>
      <location filename="init_draft_statusbar.py" line="151"/>
      <source>Set custom annotation scale in format x:x, x=x</source>
      <translation>Ustaw niestandardową skalę dla adnotacji w formacie x:x, x=x</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="83"/>
      <source>Task panel:</source>
      <translation>Panel zadań:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="205"/>
      <source>At least one element must be selected.</source>
      <translation>Należy wybrać co najmniej jeden element.</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="216"/>
      <source>Selection is not suitable for array.</source>
      <translation>Wybór nie jest odpowiedni dla tablicy.</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="379"/>
      <source>Object:</source>
      <translation>Obiekt:</translation>
    </message>
    <message>
      <location filename="task_polararray.py" line="203"/>
      <source>Number of elements must be at least 2.</source>
      <translation>Liczba elementów musi wynosić co najmniej 2.</translation>
    </message>
    <message>
      <location filename="task_polararray.py" line="207"/>
      <source>The angle is above 360 degrees. It is set to this value to proceed.</source>
      <translation>Kąt jest większy niż 360 stopni. Aby kontynuować, należy ustawić go na tę wartość.</translation>
    </message>
    <message>
      <location filename="task_polararray.py" line="210"/>
      <source>The angle is below -360 degrees. It is set to this value to proceed.</source>
      <translation>Kąt jest mniejszy niż -360 stopni. Aby kontynuować, należy ustawić go na tę wartość.</translation>
    </message>
    <message>
      <location filename="task_circulararray.py" line="341"/>
      <source>Center reset:</source>
      <translation>Resetowanie środka:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="348"/>
      <source>Fuse:</source>
      <translation>Suma:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="362"/>
      <source>Create Link array:</source>
      <translation>Utwórz tablicę odnośników:</translation>
    </message>
    <message>
      <location filename="task_polararray.py" line="332"/>
      <source>Number of elements:</source>
      <translation>Liczba elementów:</translation>
    </message>
    <message>
      <location filename="task_polararray.py" line="333"/>
      <source>Polar angle:</source>
      <translation>Kąt polarny:</translation>
    </message>
    <message>
      <location filename="task_circulararray.py" line="388"/>
      <source>Center of rotation:</source>
      <translation>Środek obrotu:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="400"/>
      <source>Aborted:</source>
      <translation>Przerwano:</translation>
    </message>
    <message>
      <location filename="task_circulararray.py" line="222"/>
      <source>Number of layers must be at least 2.</source>
      <translation>Liczba warstw musi wynosić co najmniej 2.</translation>
    </message>
    <message>
      <location filename="task_circulararray.py" line="234"/>
      <source>Radial distance is zero. Resulting array may not look correct.</source>
      <translation>Odległość radialna wynosi zero. Wynikowa tablica może nie wyglądać poprawnie.</translation>
    </message>
    <message>
      <location filename="task_circulararray.py" line="236"/>
      <source>Radial distance is negative. It is made positive to proceed.</source>
      <translation>Odległość promieniowa jest ujemna. Należy ją uczynić dodatnią, aby kontynuować.</translation>
    </message>
    <message>
      <location filename="task_circulararray.py" line="240"/>
      <source>Tangential distance cannot be zero.</source>
      <translation>Odległość styczna nie może wynosić zero.</translation>
    </message>
    <message>
      <location filename="task_circulararray.py" line="243"/>
      <source>Tangential distance is negative. It is made positive to proceed.</source>
      <translation>Styczna odległość jest ujemna. Należy ją uczynić dodatnią, aby kontynuować.</translation>
    </message>
    <message>
      <location filename="task_circulararray.py" line="383"/>
      <source>Radial distance:</source>
      <translation>Dystans promieniowy:</translation>
    </message>
    <message>
      <location filename="task_circulararray.py" line="384"/>
      <source>Tangential distance:</source>
      <translation>Odległość styczna:</translation>
    </message>
    <message>
      <location filename="task_circulararray.py" line="385"/>
      <source>Number of circular layers:</source>
      <translation>Liczba warstw kołowych:</translation>
    </message>
    <message>
      <location filename="task_circulararray.py" line="386"/>
      <source>Symmetry parameter:</source>
      <translation>Parametry symetrii:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="209"/>
      <source>Number of elements must be at least 1.</source>
      <translation>Liczba elementów musi wynosić co najmniej 1.</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="320"/>
      <source>Interval X reset:</source>
      <translation>Odstęp X zresetowany:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="329"/>
      <source>Interval Y reset:</source>
      <translation>Odstęp Y zresetowany:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="338"/>
      <source>Interval Z reset:</source>
      <translation>Odstęp Z zresetowany:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="380"/>
      <source>Number of X elements:</source>
      <translation>Liczba elementów X:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="382"/>
      <source>Interval X:</source>
      <translation>Odstęp X:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="385"/>
      <source>Number of Y elements:</source>
      <translation>Liczba elementów Y:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="387"/>
      <source>Interval Y:</source>
      <translation>Odstęp Y:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="390"/>
      <source>Number of Z elements:</source>
      <translation>Liczba elementów Z:</translation>
    </message>
    <message>
      <location filename="task_orthoarray.py" line="392"/>
      <source>Interval Z:</source>
      <translation>Odstęp Z:</translation>
    </message>
    <message>
      <location filename="gui_shapestrings.py" line="77"/>
      <source>ShapeString</source>
      <translation>Kształt z tekstu</translation>
    </message>
    <message>
      <location filename="task_shapestring.py" line="76"/>
      <source>Default</source>
      <translation>Domyślne</translation>
    </message>
    <message>
      <location filename="task_shapestring.py" line="88"/>
      <source>Pick ShapeString location point:</source>
      <translation>Wybierz punkt lokalizacji łańcucha kształtu:</translation>
    </message>
    <message>
      <location filename="gui_shapestrings.py" line="143"/>
      <source>Create ShapeString</source>
      <translation>Utwórz ciąg kształtów</translation>
    </message>
    <message>
      <location filename="gui_downgrade.py" line="85"/>
      <source>Downgrade</source>
      <translation type="unfinished">Downgrade</translation>
    </message>
    <message>
      <location filename="gui_upgrade.py" line="67"/>
      <source>Select an object to upgrade</source>
      <translation>Wybierz obiekt do uaktualnienia</translation>
    </message>
    <message>
      <location filename="gui_clone.py" line="75"/>
      <source>Clone</source>
      <translation>Klon</translation>
    </message>
    <message>
      <location filename="gui_clone.py" line="79"/>
      <source>Select an object to clone</source>
      <translation>Wybierz obiekt do klonowania</translation>
    </message>
    <message>
      <location filename="gui_ellipses.py" line="65"/>
      <source>Ellipse</source>
      <translation>Elipsa</translation>
    </message>
    <message>
      <location filename="gui_rectangles.py" line="70"/>
      <source>Pick first point</source>
      <translation>Wybierz pierwszy punkt</translation>
    </message>
    <message>
      <location filename="gui_ellipses.py" line="140"/>
      <source>Create Ellipse</source>
      <translation>Utwórz elipsę</translation>
    </message>
    <message>
      <location filename="gui_rectangles.py" line="203"/>
      <source>Pick opposite point</source>
      <translation>Wybierz przeciwległy punkt</translation>
    </message>
    <message>
      <location filename="gui_lines.py" line="169"/>
      <source>Create Line</source>
      <translation>Utwórz linię</translation>
    </message>
    <message>
      <location filename="gui_lines.py" line="191"/>
      <source>Create Wire</source>
      <translation>Utwórz linię łamaną</translation>
    </message>
    <message>
      <location filename="gui_splines.py" line="140"/>
      <source>Pick next point</source>
      <translation>Wybierz kolejny punkt</translation>
    </message>
    <message>
      <location filename="gui_lines.py" line="336"/>
      <source>Unable to create a Wire from selected objects</source>
      <translation>Nie można utworzyć linii łamanej z zaznaczonych obiektów</translation>
    </message>
    <message>
      <location filename="gui_lines.py" line="358"/>
      <source>Convert to Wire</source>
      <translation>Konwertuj do Linii łamanej</translation>
    </message>
    <message>
      <location filename="gui_edit_draft_objects.py" line="507"/>
      <source>This object does not support possible coincident points, please try again.</source>
      <translation>Ten obiekt nie obsługuje możliwych punktów zbieżnych, proszę spróbować ponownie.</translation>
    </message>
    <message>
      <location filename="gui_edit_draft_objects.py" line="557"/>
      <source>Active object must have more than two points/nodes</source>
      <translation>Aktywny obiekt musi mieć więcej niż dwa punkty/węzły</translation>
    </message>
    <message>
      <location filename="gui_edit_draft_objects.py" line="671"/>
      <source>Selection is not a Knot</source>
      <translation>Zaznaczenie nie jest Węzłem</translation>
    </message>
    <message>
      <location filename="gui_edit_draft_objects.py" line="698"/>
      <source>Endpoint of BezCurve can't be smoothed</source>
      <translation>Punkt końcowy krzywej Beziera nie może być wygładzony</translation>
    </message>
    <message>
      <location filename="gui_edit_sketcher_objects.py" line="60"/>
      <source>Sketch is too complex to edit: it is suggested to use sketcher default editor</source>
      <translation>Szkic jest zbyt złożony, aby edytować: sugeruje się użycie domyślnego edytora szkicownika</translation>
    </message>
    <message>
      <location filename="gui_facebinders.py" line="66"/>
      <source>Facebinder</source>
      <translation type="unfinished">Facebinder</translation>
    </message>
    <message>
      <location filename="gui_facebinders.py" line="71"/>
      <source>Select faces from existing objects</source>
      <translation>Wybierz ścianę na istniejącym obiekcie</translation>
    </message>
    <message>
      <location filename="gui_lineslope.py" line="64"/>
      <source>Change slope</source>
      <translation>Zmień nachylenie</translation>
    </message>
    <message>
      <location filename="gui_subelements.py" line="70"/>
      <source>Subelement highlight</source>
      <translation>Podświetlenie części obiektu</translation>
    </message>
    <message>
      <location filename="gui_subelements.py" line="109"/>
      <source>Select an object to edit</source>
      <translation>Wybierz obiekt do edycji</translation>
    </message>
    <message>
      <location filename="gui_dimensions.py" line="89"/>
      <source>Dimension</source>
      <translation>Wymiar</translation>
    </message>
    <message>
      <location filename="gui_dimensions.py" line="294"/>
      <source>Create Dimension</source>
      <translation>Utwórz wymiar</translation>
    </message>
    <message>
      <location filename="gui_dimensions.py" line="309"/>
      <source>Create Dimension (radial)</source>
      <translation>Utwórz wymiar (promieniowy)</translation>
    </message>
    <message>
      <location filename="gui_dimensions.py" line="511"/>
      <source>Edges don't intersect!</source>
      <translation>Krawędzie nie przecinają się!</translation>
    </message>
    <message>
      <location filename="gui_drawing.py" line="76"/>
      <source>Drawing</source>
      <translation>Rysunek</translation>
    </message>
    <message>
      <location filename="gui_drawing.py" line="77"/>
      <source>The Drawing Workbench is obsolete since 0.17, consider using the TechDraw Workbench instead.</source>
      <translation>Środowisko pracy Drawing jest przestarzałe od wersji 0.17, rozważ użycie Środowiska TechDraw zamiast niego.</translation>
    </message>
    <message>
      <location filename="gui_shape2dview.py" line="70"/>
      <source>Select an object to project</source>
      <translation>Wybierz obiekt do projektowania</translation>
    </message>
    <message>
      <location filename="gui_annotationstyleeditor.py" line="68"/>
      <source>Annotation style editor</source>
      <translation>Edytor stylów adnotacji</translation>
    </message>
    <message>
      <location filename="gui_annotationstyleeditor.py" line="292"/>
      <source>Open styles file</source>
      <translation>Otwórz plik stylów</translation>
    </message>
    <message>
      <location filename="gui_annotationstyleeditor.py" line="316"/>
      <source>JSON file (*.json)</source>
      <translation>Plik JSON (*.json)</translation>
    </message>
    <message>
      <location filename="gui_annotationstyleeditor.py" line="314"/>
      <source>Save styles file</source>
      <translation>Zapisz plik stylów</translation>
    </message>
    <message>
      <location filename="gui_upgrade.py" line="85"/>
      <source>Upgrade</source>
      <translation>Ulepsz</translation>
    </message>
    <message>
      <location filename="gui_move.py" line="205"/>
      <source>Move</source>
      <translation>Przesuń</translation>
    </message>
    <message>
      <location filename="gui_move.py" line="82"/>
      <source>Select an object to move</source>
      <translation>Zaznacz obiekt do przeniesienia</translation>
    </message>
    <message>
      <location filename="gui_move.py" line="104"/>
      <source>Pick start point</source>
      <translation>Wybierz punkt początkowy</translation>
    </message>
    <message>
      <location filename="gui_move.py" line="303"/>
      <source>Pick end point</source>
      <translation>Wybierz punkt końcowy</translation>
    </message>
    <message>
      <location filename="gui_rotate.py" line="283"/>
      <source>Some subelements could not be moved.</source>
      <translation>Niektóre elementy podrzędne nie mogły być przeniesione.</translation>
    </message>
    <message>
      <location filename="gui_pointarray.py" line="123"/>
      <source>Point array</source>
      <translation>Szyk punktów</translation>
    </message>
    <message>
      <location filename="gui_pointarray.py" line="105"/>
      <source>Please select exactly two objects, the base object and the point object, before calling this command.</source>
      <translation>Przed wywołaniem tego polecenia proszę wybrać dokładnie dwa obiekty, obiekt bazowy i punkt.</translation>
    </message>
    <message>
      <location filename="gui_togglemodes.py" line="73"/>
      <source>No active Draft Toolbar.</source>
      <translation>Brak aktywnego paska narzędzi Rysuj.</translation>
    </message>
    <message>
      <location filename="gui_togglemodes.py" line="93"/>
      <source>Construction mode</source>
      <translation>Tryb konstrukcji</translation>
    </message>
    <message>
      <location filename="gui_togglemodes.py" line="125"/>
      <source>Continue mode</source>
      <translation>Tryb kontynuowania</translation>
    </message>
    <message>
      <location filename="gui_togglemodes.py" line="159"/>
      <source>Toggle display mode</source>
      <translation>Przełącz tryb wyświetlania</translation>
    </message>
    <message>
      <location filename="gui_wire2spline.py" line="79"/>
      <source>Convert polyline/B-spline</source>
      <translation>Konwertuj polilinię/B-spline</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="109"/>
      <source>Main toggle snap</source>
      <translation>Główny przełącznik przyciągania</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="140"/>
      <source>Midpoint snap</source>
      <translation>Przyciąganie do punktu środkowego</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="170"/>
      <source>Perpendicular snap</source>
      <translation>Przyciąganie prostopadle</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="200"/>
      <source>Grid snap</source>
      <translation>Przyciąganie do siatki</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="230"/>
      <source>Intersection snap</source>
      <translation>Przyciąganie do punktu przecięcia</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="260"/>
      <source>Parallel snap</source>
      <translation>Przyciąganie równoległe</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="290"/>
      <source>Endpoint snap</source>
      <translation>Przyciąganie do punktu końcowego</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="321"/>
      <source>Angle snap (30 and 45 degrees)</source>
      <translation>Przyciąganie do kąta (30 i 45 stopni)</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="351"/>
      <source>Arc center snap</source>
      <translation>Przyciąganie do środka łuku</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="381"/>
      <source>Edge extension snap</source>
      <translation>Przyciąganie do wydłużenia krawędzi</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="411"/>
      <source>Near snap</source>
      <translation>Przyciąganie do najbliższego</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="442"/>
      <source>Orthogonal snap</source>
      <translation>Przyciąganie ortogonalne</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="472"/>
      <source>Special point snap</source>
      <translation>Przyciąganie do punktów specjalnych</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="503"/>
      <source>Dimension display</source>
      <translation>Wyświetlanie wymiarów</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="536"/>
      <source>Working plane snap</source>
      <translation>Przyciąganie do płaszczyzny roboczej</translation>
    </message>
    <message>
      <location filename="gui_snaps.py" line="566"/>
      <source>Show snap toolbar</source>
      <translation>Pokaż pasek narzędzi przyciągania</translation>
    </message>
    <message>
      <location filename="gui_array_simple.py" line="101"/>
      <source>Array</source>
      <translation>Tablica</translation>
    </message>
    <message>
      <location filename="gui_array_simple.py" line="77"/>
      <source>Select an object to array</source>
      <translation>Wybierz obiekt do stworzenia szyku</translation>
    </message>
    <message>
      <location filename="gui_array_simple.py" line="124"/>
      <source>Link array</source>
      <translation>Szyk liniowy</translation>
    </message>
    <message>
      <location filename="gui_polygons.py" line="63"/>
      <source>Polygon</source>
      <translation>Wielokąt</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="88"/>
      <source>Pick center point</source>
      <translation>Wybierz punkt środkowy</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="410"/>
      <source>Pick radius</source>
      <translation>Wybierz promień</translation>
    </message>
    <message>
      <location filename="gui_polygons.py" line="224"/>
      <source>Create Polygon (Part)</source>
      <translation>Utwórz wielokąt (Środowisko pracy Część)</translation>
    </message>
    <message>
      <location filename="gui_polygons.py" line="243"/>
      <source>Create Polygon</source>
      <translation>Utwórz wielokąt</translation>
    </message>
    <message>
      <location filename="gui_mirror.py" line="124"/>
      <source>Mirror</source>
      <translation>Odbicie lustrzane</translation>
    </message>
    <message>
      <location filename="gui_mirror.py" line="74"/>
      <source>Select an object to mirror</source>
      <translation>Wybierz obiekt do stworzenia odbicia lustrzanego</translation>
    </message>
    <message>
      <location filename="gui_mirror.py" line="94"/>
      <source>Pick start point of mirror line</source>
      <translation>Wybierz punkt początkowy dla linii odbicia</translation>
    </message>
    <message>
      <location filename="gui_mirror.py" line="205"/>
      <source>Pick end point of mirror line</source>
      <translation>Wybierz punkt końcowy linii odbicia</translation>
    </message>
    <message>
      <location filename="gui_points.py" line="146"/>
      <source>Create Point</source>
      <translation>Utwórz punkt</translation>
    </message>
    <message>
      <location filename="gui_scale.py" line="352"/>
      <source>Scale</source>
      <translation>Skala</translation>
    </message>
    <message>
      <location filename="gui_scale.py" line="88"/>
      <source>Select an object to scale</source>
      <translation>Wybierz obiekt do modyfikacji skali</translation>
    </message>
    <message>
      <location filename="gui_scale.py" line="109"/>
      <source>Pick base point</source>
      <translation>Wybierz punkt bazowy</translation>
    </message>
    <message>
      <location filename="gui_scale.py" line="131"/>
      <source>Pick reference distance from base point</source>
      <translation>Wybierz odległość odniesienia od punktu bazowego</translation>
    </message>
    <message>
      <location filename="gui_scale.py" line="204"/>
      <source>Some subelements could not be scaled.</source>
      <translation>Niektóre elementy podrzędne nie mogły być skalowane.</translation>
    </message>
    <message>
      <location filename="gui_scale.py" line="334"/>
      <source>Unable to scale object: </source>
      <translation>Nie można ustawić skali obiektu: </translation>
    </message>
    <message>
      <location filename="gui_scale.py" line="337"/>
      <source>Unable to scale objects: </source>
      <translation>Nie można ustawić skali obiektów: </translation>
    </message>
    <message>
      <location filename="gui_scale.py" line="339"/>
      <source>This object type cannot be scaled directly. Please use the clone method.</source>
      <translation>Tego typu obiektu nie można bezpośrednio przeskalowywać. Proszę użyć metody klonowania.</translation>
    </message>
    <message>
      <location filename="gui_scale.py" line="400"/>
      <source>Pick new distance from base point</source>
      <translation>Wybierz nową odległość od punktu bazowego</translation>
    </message>
    <message>
      <location filename="gui_shape2dview.py" line="66"/>
      <source>Project 2D view</source>
      <translation>Widok 2D projektu</translation>
    </message>
    <message>
      <location filename="gui_shape2dview.py" line="111"/>
      <source>Create 2D view</source>
      <translation>Utwórz widok 2D</translation>
    </message>
    <message>
      <location filename="gui_beziers.py" line="332"/>
      <source>Bezier curve has been closed</source>
      <translation>Krzywa Beziera została zamknięta</translation>
    </message>
    <message>
      <location filename="gui_splines.py" line="131"/>
      <source>Last point has been removed</source>
      <translation>Ostatni punkt został usunięty</translation>
    </message>
    <message>
      <location filename="gui_splines.py" line="147"/>
      <source>Pick next point, or finish (A) or close (O)</source>
      <translation>Wybierz następny punkt, lub zakończ (A) lub zamknij (O)</translation>
    </message>
    <message>
      <location filename="gui_beziers.py" line="451"/>
      <source>Create BezCurve</source>
      <translation>Utwórz krzywą Beziera</translation>
    </message>
    <message>
      <location filename="gui_beziers.py" line="376"/>
      <source>Click and drag to define next knot</source>
      <translation>Kliknij i przeciągnij, aby zdefiniować następny węzeł</translation>
    </message>
    <message>
      <location filename="gui_beziers.py" line="382"/>
      <source>Click and drag to define next knot, or finish (A) or close (O)</source>
      <translation>Kliknij i przeciągnij, aby zdefiniować następny węzeł, lub zakończ (A) lub zamknij (O)</translation>
    </message>
    <message>
      <location filename="gui_dimension_ops.py" line="55"/>
      <source>Flip dimension</source>
      <translation>Odwróć wymiar</translation>
    </message>
    <message>
      <location filename="gui_stretch.py" line="470"/>
      <source>Stretch</source>
      <translation>Rozciągnij</translation>
    </message>
    <message>
      <location filename="gui_stretch.py" line="73"/>
      <source>Select an object to stretch</source>
      <translation>Wybierz obiekt do rozciągnięcia</translation>
    </message>
    <message>
      <location filename="gui_stretch.py" line="122"/>
      <source>Pick first point of selection rectangle</source>
      <translation>Wybierz pierwszy punkt ramki zaznaczenia</translation>
    </message>
    <message>
      <location filename="gui_stretch.py" line="159"/>
      <source>Pick opposite point of selection rectangle</source>
      <translation>Wybierz przeciwny punkt ramki zaznaczenia</translation>
    </message>
    <message>
      <location filename="gui_stretch.py" line="168"/>
      <source>Pick start point of displacement</source>
      <translation>Wybierz punkt początkowy przemieszczenia</translation>
    </message>
    <message>
      <location filename="gui_stretch.py" line="231"/>
      <source>Pick end point of displacement</source>
      <translation>Wybierz punkt końcowy przemieszczenia</translation>
    </message>
    <message>
      <location filename="gui_stretch.py" line="443"/>
      <source>Turning one Rectangle into a Wire</source>
      <translation>Przekształcanie jednego prostokąta w linię łamaną</translation>
    </message>
    <message>
      <location filename="gui_grid.py" line="51"/>
      <source>Toggle grid</source>
      <translation>Przełącz siatkę</translation>
    </message>
    <message>
      <location filename="gui_rectangles.py" line="60"/>
      <source>Rectangle</source>
      <translation>Prostokąt</translation>
    </message>
    <message>
      <location filename="gui_rectangles.py" line="126"/>
      <source>Create Plane</source>
      <translation>Utwórz płaszczyznę</translation>
    </message>
    <message>
      <location filename="gui_rectangles.py" line="143"/>
      <source>Create Rectangle</source>
      <translation>Utwórz prostokąt</translation>
    </message>
    <message>
      <location filename="gui_draft2sketch.py" line="143"/>
      <source>Convert Draft/Sketch</source>
      <translation>Konwertuj rysunek roboczy do szkicu</translation>
    </message>
    <message>
      <location filename="gui_draft2sketch.py" line="66"/>
      <source>Select an object to convert.</source>
      <translation>Wybierz obiekt do konwersji.</translation>
    </message>
    <message>
      <location filename="gui_draft2sketch.py" line="99"/>
      <source>Convert to Sketch</source>
      <translation>Konwertuj na szkic</translation>
    </message>
    <message>
      <location filename="gui_draft2sketch.py" line="114"/>
      <source>Convert to Draft</source>
      <translation>Konwertuj na rysunek roboczy</translation>
    </message>
    <message>
      <location filename="gui_heal.py" line="51"/>
      <source>Heal</source>
      <translation>Wylecz</translation>
    </message>
    <message>
      <location filename="gui_labels.py" line="81"/>
      <source>Pick target point</source>
      <translation>Wybierz punkt docelowy</translation>
    </message>
    <message>
      <location filename="gui_labels.py" line="158"/>
      <source>Create Label</source>
      <translation>Utwórz etykietę</translation>
    </message>
    <message>
      <location filename="gui_labels.py" line="219"/>
      <source>Pick endpoint of leader line</source>
      <translation>Wybierz punkt końcowy linii odniesienia</translation>
    </message>
    <message>
      <location filename="gui_labels.py" line="229"/>
      <source>Pick text position</source>
      <translation>Wybierz pozycję tekstu</translation>
    </message>
    <message>
      <location filename="gui_edit.py" line="305"/>
      <source>Select a Draft object to edit</source>
      <translation>Wybierz obiekt rysunku roboczego do edycji</translation>
    </message>
    <message>
      <location filename="gui_edit.py" line="560"/>
      <source>No edit point found for selected object</source>
      <translation>Nie znaleziono punktu edycji dla wybranego obiektu</translation>
    </message>
    <message>
      <location filename="gui_edit.py" line="933"/>
      <source>Too many objects selected, max number set to: </source>
      <translation>Wybrano zbyt wiele obiektów, maksymalna liczba została ustawiona na: </translation>
    </message>
    <message>
      <location filename="gui_edit.py" line="941"/>
      <source>: this object is not editable</source>
      <translation>: tego obiektu nie można edytować</translation>
    </message>
    <message>
      <location filename="gui_patharray.py" line="143"/>
      <source>Path array</source>
      <translation type="unfinished">Path array</translation>
    </message>
    <message>
      <location filename="gui_pathtwistedarray.py" line="79"/>
      <source>Please select exactly two objects, the base object and the path object, before calling this command.</source>
      <translation>Przed wywołaniem tego polecenia należy wybrać dokładnie dwa obiekty, obiekt bazowy i obiekt ścieżki.</translation>
    </message>
    <message>
      <location filename="gui_patharray.py" line="168"/>
      <source>Path link array</source>
      <translation>Utwórz tablicę odnośników</translation>
    </message>
    <message>
      <location filename="gui_pathtwistedarray.py" line="101"/>
      <source>Path twisted array</source>
      <translation type="unfinished">Path twisted array</translation>
    </message>
    <message>
      <location filename="gui_pathtwistedarray.py" line="127"/>
      <source>Path twisted link array</source>
      <translation type="unfinished">Path twisted link array</translation>
    </message>
    <message>
      <location filename="gui_trimex.py" line="81"/>
      <source>Trimex</source>
      <translation type="unfinished">Trimex</translation>
    </message>
    <message>
      <location filename="gui_trimex.py" line="91"/>
      <source>Select objects to trim or extend</source>
      <translation>Wybierz obiekty do przycięcia lub wydłużenia</translation>
    </message>
    <message>
      <location filename="gui_offset.py" line="140"/>
      <source>Pick distance</source>
      <translation>Wybierz odległość</translation>
    </message>
    <message>
      <location filename="gui_trimex.py" line="320"/>
      <source>The offset distance</source>
      <translation>Odległość przesunięcia</translation>
    </message>
    <message>
      <location filename="gui_trimex.py" line="336"/>
      <source>The offset angle</source>
      <translation>Kąt przesunięcia</translation>
    </message>
    <message>
      <location filename="gui_trimex.py" line="474"/>
      <source>Unable to trim these objects, only Draft wires and arcs are supported.</source>
      <translation>Nie można przyciąć tych obiektów, obsługiwane są tylko linie łamane i łuki.</translation>
    </message>
    <message>
      <location filename="gui_trimex.py" line="479"/>
      <source>Unable to trim these objects, too many wires</source>
      <translation>Nie można przyciąć tych obiektów, zbyt wiele linii łamanych</translation>
    </message>
    <message>
      <location filename="gui_trimex.py" line="496"/>
      <source>These objects don't intersect.</source>
      <translation>Te obiekty nie przecinają się.</translation>
    </message>
    <message>
      <location filename="gui_trimex.py" line="499"/>
      <source>Too many intersection points.</source>
      <translation>Zbyt wiele punktów przecięcia.</translation>
    </message>
    <message>
      <location filename="gui_join.py" line="71"/>
      <source>Join</source>
      <translation>Połącz</translation>
    </message>
    <message>
      <location filename="gui_join.py" line="76"/>
      <source>Select an object to join</source>
      <translation>Wybierz obiekt do dołączenia</translation>
    </message>
    <message>
      <location filename="gui_join.py" line="99"/>
      <source>Join lines</source>
      <translation>Połącz linie</translation>
    </message>
    <message>
      <location filename="gui_join.py" line="110"/>
      <source>Selection:</source>
      <translation>Wybór:</translation>
    </message>
    <message>
      <location filename="gui_splines.py" line="120"/>
      <source>Spline has been closed</source>
      <translation>Splajn został zamknięty</translation>
    </message>
    <message>
      <location filename="gui_splines.py" line="183"/>
      <source>Create B-spline</source>
      <translation>Utwórz B-spline</translation>
    </message>
    <message>
      <location filename="gui_selectplane.py" line="89"/>
      <source>Pick a face, 3 vertices or a WP Proxy to define the drawing plane</source>
      <translation>Wybierz płaszczyznę, 3 wierzchołki lub proxy WP, aby zdefiniować płaszczyznę rysowania</translation>
    </message>
    <message>
      <location filename="gui_selectplane.py" line="263"/>
      <source>Working plane aligned to global placement of</source>
      <translation>Płaszczyzna robocza wyrównana do położenia globalnego</translation>
    </message>
    <message>
      <location filename="gui_selectplane.py" line="512"/>
      <source>Dir</source>
      <translation>Katalog</translation>
    </message>
    <message>
      <location filename="gui_selectplane.py" line="528"/>
      <source>Custom</source>
      <translation>Niestandardowe</translation>
    </message>
    <message>
      <location filename="gui_lineops.py" line="73"/>
      <source>No active command.</source>
      <translation>Brak aktywnego polecenia.</translation>
    </message>
    <message>
      <location filename="gui_lineops.py" line="92"/>
      <source>Finish line</source>
      <translation>Zakończ linię</translation>
    </message>
    <message>
      <location filename="gui_lineops.py" line="118"/>
      <source>Close line</source>
      <translation>Zamknij linię</translation>
    </message>
    <message>
      <location filename="gui_lineops.py" line="144"/>
      <source>Undo line</source>
      <translation>Cofnij linię</translation>
    </message>
    <message>
      <location filename="gui_split.py" line="61"/>
      <source>Split</source>
      <translation>Rozdziel</translation>
    </message>
    <message>
      <location filename="gui_split.py" line="64"/>
      <source>Click anywhere on a line to split it.</source>
      <translation>Kliknij gdziekolwiek na linii, aby ją podzielić.</translation>
    </message>
    <message>
      <location filename="gui_split.py" line="106"/>
      <source>Split line</source>
      <translation>Podziel linię</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="76"/>
      <source>Fillet radius</source>
      <translation>Promień zaokrąglenia</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="79"/>
      <source>Radius of fillet</source>
      <translation>Promień zaokrąglenia</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="112"/>
      <source>Enter radius.</source>
      <translation>Wprowadź promień.</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="131"/>
      <source>Delete original objects:</source>
      <translation>Usuń oryginalne obiekty:</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="136"/>
      <source>Chamfer mode:</source>
      <translation>Tryb tworzenia fazki:</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="153"/>
      <source>Two elements needed.</source>
      <translation>Potrzebne są dwa elementy.</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="160"/>
      <source>Test object</source>
      <translation>Badany obiekt</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="161"/>
      <source>Test object removed</source>
      <translation>Obiekt testowy został usunięty</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="163"/>
      <source>Fillet cannot be created</source>
      <translation>Zaokrąglenie nie może być utworzone</translation>
    </message>
    <message>
      <location filename="gui_fillets.py" line="193"/>
      <source>Create fillet</source>
      <translation>Utwórz zaokrąglenie</translation>
    </message>
    <message>
      <location filename="gui_shapestrings.py" line="102"/>
      <source>Pick ShapeString location point</source>
      <translation>Wskaż punk lokalizacji formy tekstowej</translation>
    </message>
    <message>
      <location filename="gui_styles.py" line="55"/>
      <source>Apply style</source>
      <translation>Zastosuj styl</translation>
    </message>
    <message>
      <location filename="gui_styles.py" line="75"/>
      <source>Change Style</source>
      <translation>Zmień styl</translation>
    </message>
    <message>
      <location filename="gui_groups.py" line="64"/>
      <source>Add to group</source>
      <translation>Dodaj do grupy</translation>
    </message>
    <message>
      <location filename="gui_groups.py" line="159"/>
      <source>Select group</source>
      <translation>Wybierz grupę</translation>
    </message>
    <message>
      <location filename="gui_groups.py" line="232"/>
      <source>Autogroup</source>
      <translation type="unfinished">Autogroup</translation>
    </message>
    <message>
      <location filename="gui_groups.py" line="282"/>
      <source>Add new Layer</source>
      <translation>Dodaj nową warstwę</translation>
    </message>
    <message>
      <location filename="gui_groups.py" line="336"/>
      <source>Add to construction group</source>
      <translation>Dodaj do grupy konstrukcyjnej</translation>
    </message>
    <message>
      <location filename="gui_offset.py" line="76"/>
      <source>Select an object to offset</source>
      <translation>Wybierz obiekt do przesunięcia</translation>
    </message>
    <message>
      <location filename="gui_offset.py" line="82"/>
      <source>Offset only works on one object at a time.</source>
      <translation>Przesunięcie działa tylko na jednym obiekcie w danym czasie.</translation>
    </message>
    <message>
      <location filename="gui_offset.py" line="92"/>
      <source>Cannot offset this object type</source>
      <translation>Nie można przesunąć tego typu obiektu</translation>
    </message>
    <message>
      <location filename="gui_offset.py" line="120"/>
      <source>Offset of Bezier curves is currently not supported</source>
      <translation>Przesunięcie krzywych Beziera nie jest obecnie obsługiwane</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="447"/>
      <source>Start angle</source>
      <translation>Kąt początkowy</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="452"/>
      <source>Pick start angle</source>
      <translation>Wybierz kąt początkowy</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="455"/>
      <source>Aperture angle</source>
      <translation type="unfinished">Aperture angle</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="291"/>
      <source>Pick aperture</source>
      <translation type="unfinished">Pick aperture</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="316"/>
      <source>Create Circle (Part)</source>
      <translation>Utwórz okrąg (Część)</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="335"/>
      <source>Create Circle</source>
      <translation>Utwórz okrąg</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="369"/>
      <source>Create Arc (Part)</source>
      <translation>Utwórz łuk (Część)</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="389"/>
      <source>Create Arc</source>
      <translation>Utwórz łuk</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="466"/>
      <source>Pick aperture angle</source>
      <translation type="unfinished">Pick aperture angle</translation>
    </message>
    <message>
      <location filename="gui_arcs.py" line="481"/>
      <source>Arc by 3 points</source>
      <translation>Łuk przez 3 punkty</translation>
    </message>
    <message>
      <location filename="gui_texts.py" line="66"/>
      <source>Text</source>
      <translation>Tekst</translation>
    </message>
    <message>
      <location filename="gui_texts.py" line="76"/>
      <source>Pick location point</source>
      <translation>Wybierz lokalizację punktu</translation>
    </message>
    <message>
      <location filename="gui_texts.py" line="116"/>
      <source>Create Text</source>
      <translation>Utwórz tekst</translation>
    </message>
    <message>
      <location filename="gui_rotate.py" line="280"/>
      <source>Rotate</source>
      <translation>Obróć</translation>
    </message>
    <message>
      <location filename="gui_rotate.py" line="79"/>
      <source>Select an object to rotate</source>
      <translation>Zaznacz obiekt do obrócenia</translation>
    </message>
    <message>
      <location filename="gui_rotate.py" line="101"/>
      <source>Pick rotation center</source>
      <translation>Wybierz środek obrotu</translation>
    </message>
    <message>
      <location filename="gui_rotate.py" line="390"/>
      <source>Base angle</source>
      <translation>Kąt bazowy</translation>
    </message>
    <message>
      <location filename="gui_rotate.py" line="391"/>
      <source>The base angle you wish to start the rotation from</source>
      <translation>Kąt początkowy, od którego chcesz rozpocząć obrót</translation>
    </message>
    <message>
      <location filename="gui_rotate.py" line="394"/>
      <source>Pick base angle</source>
      <translation>Wybierz kąt początkowy</translation>
    </message>
    <message>
      <location filename="gui_rotate.py" line="403"/>
      <source>Rotation</source>
      <translation>Obrót</translation>
    </message>
    <message>
      <location filename="gui_rotate.py" line="404"/>
      <source>The amount of rotation you wish to perform.
The final angle will be the base angle plus this amount.</source>
      <translation>Zakres obrotu, który chcesz wykonać.
Kąt końcowy będzie równy kątowi podstawowemu plus ta wartość.</translation>
    </message>
    <message>
      <location filename="gui_rotate.py" line="412"/>
      <source>Pick rotation angle</source>
      <translation>Wybierz kąt obrotu</translation>
    </message>
  </context>
  <context>
    <name>importOCA</name>
    <message>
      <location filename="importOCA.py" line="362"/>
      <source>OCA error: couldn't determine character encoding</source>
      <translation>Błąd OCA: nie można określić standardu kodowania znaków</translation>
    </message>
    <message>
      <location filename="importOCA.py" line="447"/>
      <source>OCA: found no data to export</source>
      <translation>OCA: nie znaleziono danych do eksportu</translation>
    </message>
    <message>
      <location filename="importOCA.py" line="492"/>
      <source>successfully exported </source>
      <translation>wyeksportowano pomyślnie </translation>
    </message>
  </context>
</TS>
