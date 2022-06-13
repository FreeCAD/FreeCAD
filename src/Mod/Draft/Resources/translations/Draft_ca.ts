<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ca" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../draftobjects/wire.py" line="49"/>
      <source>The vertices of the wire</source>
      <translation>Els vèrtexs del filferro</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="53"/>
      <source>If the wire is closed or not</source>
      <translation>Si el filferro és tancat o no</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="57"/>
      <source>The base object is the wire, it's formed from 2 objects</source>
      <translation>L'objecte de base es el filferro, està format per 2 objectes</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="61"/>
      <source>The tool object is the wire, it's formed from 2 objects</source>
      <translation>L'objecte eina és el filferro, està format per 2 objectes</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="65"/>
      <source>The start point of this line</source>
      <translation>Punt d'inici d'aquesta línia</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="69"/>
      <source>The end point of this line</source>
      <translation>Punt de final d'aquesta línia</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="73"/>
      <source>The length of this line</source>
      <translation>Longitud d'aquesta línia</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="52"/>
      <location filename="../../draftobjects/polygon.py" line="60"/>
      <location filename="../../draftobjects/wire.py" line="77"/>
      <source>Radius to use to fillet the corners</source>
      <translation>Radi a utilitzar per arrodonir les cantonades</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="55"/>
      <location filename="../../draftobjects/polygon.py" line="64"/>
      <location filename="../../draftobjects/wire.py" line="81"/>
      <source>Size of the chamfer to give to the corners</source>
      <translation>Mida del xamfrà a donar a les cantonades</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="85"/>
      <source>Create a face if this object is closed</source>
      <translation>Crea una cara si aquest objecte és tancat</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="89"/>
      <source>The number of subdivisions of each edge</source>
      <translation>Nombre de subdivisions de cada aresta</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="67"/>
      <location filename="../../draftobjects/circle.py" line="62"/>
      <location filename="../../draftobjects/polygon.py" line="72"/>
      <location filename="../../draftobjects/bspline.py" line="57"/>
      <location filename="../../draftobjects/bezcurve.py" line="70"/>
      <location filename="../../draftobjects/wire.py" line="93"/>
      <source>The area of this object</source>
      <translation>La superfície d'aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="65"/>
      <source>The position of the tip of the leader line.
This point can be decorated with an arrow or another symbol.</source>
      <translation>Posició de l'extrem de la línia guia.
Aquest punt pot estar decorat amb una fletxa o altres símbols.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="79"/>
      <source>Object, and optionally subelement, whose properties will be displayed
as 'Text', depending on 'Label Type'.

'Target' won't be used if 'Label Type' is set to 'Custom'.</source>
      <translation>Objecte o, opcionalment, subelement les propietats del qual es mostraran com a 'Text', segons el 'Tipus d'etiqueta'.

No es mostrarà 'Destí' si 'Tipus d'etiqueta' s'ha establert com a 'Personalitzat'.</translation>
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
      <translation>Llista de punts que defineixen la línia guia; normalment, una llista de tres punts.

El primer punt ha de ser la posició del text, és a dir, 'Emplaçament', i l'últim punt
ha de ser l'extrem de la línia, és a dir, el punt de 'Destí'. El punt central es calcula
automàticament segons el signe i valor de 'Direcció de la recta' i 'Distància de la recta'.

Si 'Direcció de la recta' té el valor 'Personalitzat', la propietat 'Punts' pot ser una llista 
de punts arbitrària.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="124"/>
      <source>The direction of the straight segment of the leader line.

If 'Custom' is chosen, the points of the leader can be specified by
assigning a custom list to the 'Points' attribute.</source>
      <translation>Direcció del segment recte de la línia guia.

Si es tria 'Personalitzat', els punts es poden esfecificar assignant
una llista personalitzada a l'atribut 'Punts'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="142"/>
      <source>The length of the straight segment of the leader line.

This is an oriented distance; if it is negative, the line will be drawn
to the left or below the 'Text', otherwise to the right or above it,
depending on the value of 'Straight Direction'.</source>
      <translation>Longitud del segment recte de la línia guia.

Aquesta és una distància orientada; si el valor és negatiu, es dibuixarà
la línia cap a l'esquerra o cap avall de 'Text', altrament cap a la dreta o
damunt seu, segons el valor de 'Direcció de la recta'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="156"/>
      <source>The placement of the 'Text' element in 3D space</source>
      <translation>Posició de l'element 'Text' en l'espai 3D</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="166"/>
      <source>The text to display when 'Label Type' is set to 'Custom'</source>
      <translation>Text a mostrar quan 'Tipus d'etiqueta' s'ha establert com a 'Personalitzat'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="181"/>
      <source>The text displayed by this label.

This property is read-only, as the final text depends on 'Label Type',
and the object defined in 'Target'.
The 'Custom Text' is displayed only if 'Label Type' is set to 'Custom'.</source>
      <translation>Text que mostra aquesta etiqueta.

Aquesta propietat és de només lectura, ja que el text final depèn de 'Tipus d'etiqueta',
i de l'objecte definit a 'Destí'.
El 'Text personalitzat' només es mostrarà si 'Tipus d'etiqueta' és 'Personalitzat'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="216"/>
      <source>The type of information displayed by this label.

If 'Custom' is chosen, the contents of 'Custom Text' will be used.
For other types, the string will be calculated automatically from the object defined in 'Target'.
'Tag' and 'Material' only work for objects that have these properties, like Arch objects.

For 'Position', 'Length', and 'Area' these properties will be extracted from the main object in 'Target',
or from the subelement 'VertexN', 'EdgeN', or 'FaceN', respectively, if it is specified.</source>
      <translation>Tipus d'informació que mostrarà aquesta etiqueta.

Si es tria 'Personalitzat', es mostrarà el contingut de 'Text personalitzat'.
Per altres tipus, el text es calcularà automàticament a partir de l'objecte definit a 'Destí'.
'Etiqueta' i 'Material' només funcionen amb objectes que tinguin aquestes propietats, com
ara els objectes Arquitectura.

Les propietats 'Posició', 'Longitud' i 'Àrea' s'extrauran de l'objecte definit a 'Destí', o del
subelement 'VertexN', 'ArestaN' o 'CaraN' respectivament, si s'especifiquen.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="46"/>
      <source>The base object used by this object</source>
      <translation>L'objecte base utilitzat per aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="49"/>
      <source>The PAT file used by this object</source>
      <translation>L'arxiu PAT utilitzat per a aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="52"/>
      <source>The pattern name used by this object</source>
      <translation>El nom del patró utilitzat per a aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="55"/>
      <source>The pattern scale used by this object</source>
      <translation>L'escala del patró utilitzat per a aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="58"/>
      <source>The pattern rotation used by this object</source>
      <translation>La rotació del patró utilitzat per a aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="61"/>
      <source>If set to False, hatch is applied as is to the faces, without translation (this might give wrong results for non-XY faces)</source>
      <translation>Si s'estableix a Fals, s'aplica l'eclosió com a les cares, sense traducció (això podria donar resultats erronis per a cares no XY)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="66"/>
      <source>The linked object</source>
      <translation>L'objecte enllaçat</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="73"/>
      <source>Projection direction</source>
      <translation>Direcció de projecció</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="80"/>
      <source>The width of the lines inside this object</source>
      <translation>Amplada de les línies dins aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="88"/>
      <source>The size of the texts inside this object</source>
      <translation>La mida dels textos dins d'aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="96"/>
      <source>The spacing between lines of text</source>
      <translation>Espaiat entre línies de text</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="103"/>
      <source>The color of the projected objects</source>
      <translation>Color dels objectes projectats</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="110"/>
      <source>Shape Fill Style</source>
      <translation>Estil d'ompliment de forma</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="118"/>
      <source>Line Style</source>
      <translation>Estil de línia</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="127"/>
      <source>If checked, source objects are displayed regardless of being visible in the 3D model</source>
      <translation>Si està marcada, els objectes d'origen es mostren independentment que siguen visibles en el model 3D</translation>
    </message>
    <message>
      <location filename="../../draftobjects/circle.py" line="45"/>
      <source>Start angle of the arc</source>
      <translation>Angle inicial de l'arc</translation>
    </message>
    <message>
      <location filename="../../draftobjects/circle.py" line="50"/>
      <source>End angle of the arc (for a full circle, 
                give it same value as First Angle)</source>
      <translation>Angle final de l'arc (per a una circumferència sencera, introduïu el mateix valor que el primer angle)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/circle.py" line="54"/>
      <source>Radius of the circle</source>
      <translation>Radi de la circumferència</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="58"/>
      <location filename="../../draftobjects/circle.py" line="58"/>
      <location filename="../../draftobjects/polygon.py" line="68"/>
      <location filename="../../draftobjects/ellipse.py" line="58"/>
      <source>Create a face</source>
      <translation>Crear una cara</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="46"/>
      <source>Text string</source>
      <translation>Cadena de text</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="49"/>
      <source>Font file name</source>
      <translation>Nom del fitxer de tipus de lletra</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="52"/>
      <source>Height of text</source>
      <translation>Alçada del text</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="55"/>
      <source>Inter-character spacing</source>
      <translation>Espaiat entre caracters</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="58"/>
      <source>Fill letters with faces</source>
      <translation>Emplena les lletres amb cares</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="86"/>
      <source>The base object that will be duplicated.</source>
      <translation>L'objecte base que es duplicarà.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="93"/>
      <location filename="../../draftobjects/patharray.py" line="177"/>
      <source>The object along which the copies will be distributed. It must contain 'Edges'.</source>
      <translation>Objecte al llarg del qual es distribuiran les còpies. Ha de contenir 'Arestes'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="100"/>
      <source>Number of copies to create.</source>
      <translation>Nombre de còpies a crear.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="107"/>
      <source>Rotation factor of the twisted array.</source>
      <translation>Factor de rotació de la matriu retorçada.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="320"/>
      <location filename="../../draftobjects/pathtwistedarray.py" line="114"/>
      <location filename="../../draftobjects/pointarray.py" line="112"/>
      <location filename="../../draftobjects/patharray.py" line="208"/>
      <source>Show the individual array elements (only for Link arrays)</source>
      <translation>Mostra els elements individuals de la matriu (només per matrius de tipus 'Enllaç')</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="83"/>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="82"/>
      <source>General scaling factor that affects the annotation consistently
because it scales the text, and the line decorations, if any,
in the same proportion.</source>
      <translation>Factor d'escala genèric que afecta a les anotacions de manera consistent, ja que escala el text i les decoracions de línia, si n'hi ha, en la mateixa proporció.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="93"/>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="100"/>
      <source>Annotation style to apply to this object.
When using a saved style some of the view properties will become read-only;
they will only be editable by changing the style through the 'Annotation style editor' tool.</source>
      <translation>Tipus d'anotació que s'aplicarà a aquest objecte.
Quan s'utilitzi un estil gravat algunes de les propietats de la vista es tornaràn només de lectura; només podran fer-se editables canviant l'estil a través de l'eina 'Editor d'estils d'anotació'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draftlink.py" line="99"/>
      <source>Force sync pattern placements even when array elements are expanded</source>
      <translation>Força la sincronització de les col·locacions de patrons fins i tot quan s'expandeixen els elements de la matriu</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draftlink.py" line="112"/>
      <source>Show the individual array elements</source>
      <translation>Mostra els elements individuals de la matriu</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="46"/>
      <source>The objects included in this clone</source>
      <translation>Els objectes inclosos en aquest clon</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="51"/>
      <source>The scale factor of this clone</source>
      <translation>El factor d'escala d'aquest clon</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="57"/>
      <source>If Clones includes several objects,
set True for fusion or False for compound</source>
      <translation>Si Clons inclou diversos objectes, marqueu com a Cert per a fusionar o Fals per compondre</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="46"/>
      <source>The points of the B-spline</source>
      <translation>Punts de la B-spline</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="50"/>
      <source>If the B-spline is closed or not</source>
      <translation>Indica si la B-spline és tancada o no</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="54"/>
      <source>Create a face if this spline is closed</source>
      <translation>Crea una cara si la spline és tancada</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="67"/>
      <source>Parameterization factor</source>
      <translation>Factor de parametrització</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="57"/>
      <source>The base object this 2D view must represent</source>
      <translation>L'objecte de base que és representat per aquesta visualització 2D</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="62"/>
      <source>The projection vector of this object</source>
      <translation>El vector de projecció d'aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="68"/>
      <source>The way the viewed object must be projected</source>
      <translation>Manera en què l'objecte mostrat ha de ser projectat</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="75"/>
      <source>The indices of the faces to be projected in Individual Faces mode</source>
      <translation>Els índexs de les cares que seran projectades en el mode Cares Individuals</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="80"/>
      <source>Show hidden lines</source>
      <translation>Mostra les línies amagades</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="86"/>
      <source>Fuse wall and structure objects of same type and material</source>
      <translation>Fusiona la paret i els objectes estructurals del mateix tipus i material</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="91"/>
      <source>Tessellate Ellipses and B-splines into line segments</source>
      <translation>Fragmenta el·lipses i B-splines en segments de línia</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="98"/>
      <source>For Cutlines and Cutfaces modes, 
                    this leaves the faces at the cut location</source>
      <translation>Per als modes Tallalínies i Tallacares, això deixa les cares al lloc de tall</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="105"/>
      <source>Length of line segments if tessellating Ellipses or B-splines 
                    into line segments</source>
      <translation>Longitud dels segments de línia si es fragmenten El·lipses o B-splines en segments de línia</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="111"/>
      <source>If this is True, this object will include only visible objects</source>
      <translation>Si això està activat, aquest objecte només inclou objectes visibles</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="117"/>
      <source>A list of exclusion points. Any edge touching any of those points will not be drawn.</source>
      <translation>Una llista dels punts exclosos. Qualsevol vora que toqui qualsevol d'aquests punts no serà dibuixada.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="122"/>
      <source>If this is True, only solid geometry is handled. This overrides the base object's Only Solids property</source>
      <translation>Si això està activat, només la geometria sòlida serà tractada. Això anul·la la propietat «Sòlids» de l'objecte base</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="127"/>
      <source>If this is True, the contents are clipped to the borders of the section plane, if applicable. This overrides the base object's Clip property</source>
      <translation type="unfinished">If this is True, the contents are clipped to the borders of the section plane, if applicable. This overrides the base object's Clip property</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="132"/>
      <source>This object will be recomputed only if this is True.</source>
      <translation type="unfinished">This object will be recomputed only if this is True.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="45"/>
      <source>X Location</source>
      <translation>Posició X</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="48"/>
      <source>Y Location</source>
      <translation>Posició Y</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="51"/>
      <source>Z Location</source>
      <translation>Posició Z</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="46"/>
      <source>Length of the rectangle</source>
      <translation>Longitud del rectangle</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="49"/>
      <source>Height of the rectangle</source>
      <translation>Altura del rectangle</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="61"/>
      <source>Horizontal subdivisions of this rectangle</source>
      <translation>Subdivisions horitzontals d'aquest rectangle</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="64"/>
      <source>Vertical subdivisions of this rectangle</source>
      <translation>Subdivisions verticals d'aquest rectangle</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="43"/>
      <source>Linked faces</source>
      <translation>Cares enllaçades</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="46"/>
      <source>Specifies if splitter lines must be removed</source>
      <translation>Especifica si cal suprimir les línies de divisió</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="49"/>
      <source>An optional extrusion value to be applied to all faces</source>
      <translation>Valor opcional d'extrusió que s'aplicarà a totes les cares</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="52"/>
      <source>An optional offset value to be applied to all faces</source>
      <translation>Valor opcional de desplaçament que s'aplicarà a totes les cares</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="55"/>
      <source>This specifies if the shapes sew</source>
      <translation>Especifica si les formes seran cosides</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="58"/>
      <source>The area of the faces of this Facebinder</source>
      <translation>Àrea de les cares d'aquest Lligacares</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="48"/>
      <source>Number of faces</source>
      <translation>Nombre de cares</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="52"/>
      <source>Radius of the control circle</source>
      <translation>Radi de la circumferència de control</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="56"/>
      <source>How the polygon must be drawn from the control circle</source>
      <translation>Com cal dibuixar el polígon a partir del cercle de control</translation>
    </message>
    <message>
      <location filename="../../draftobjects/block.py" line="43"/>
      <source>The components of this block</source>
      <translation>Els components d'aquest bloc</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="47"/>
      <source>The start point of this line.</source>
      <translation>El punt inicial d'aquesta línia.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="55"/>
      <source>The end point of this line.</source>
      <translation>El punt final d'aquesta línia.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="63"/>
      <source>The length of this line.</source>
      <translation>La longitud d'aquesta línia.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="71"/>
      <source>Radius to use to fillet the corner.</source>
      <translation>Radi a utilitzar per arrodonir la cantonada.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wpproxy.py" line="42"/>
      <source>The placement of this object</source>
      <translation>La posició d'aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/layer.py" line="59"/>
      <source>The objects that are part of this layer</source>
      <translation>Els objectes que formen part d'aquesta capa</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="137"/>
      <source>The normal direction of the text of the dimension</source>
      <translation>La direcció normal del text de la dimensió</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="150"/>
      <source>The object measured by this dimension object</source>
      <translation>L'objecte mesurat per aquest objecte dimensió</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="167"/>
      <source>The object, and specific subelements of it,
that this dimension object is measuring.

There are various possibilities:
- An object, and one of its edges.
- An object, and two of its vertices.
- An arc object, and its edge.</source>
      <translation type="unfinished">The object, and specific subelements of it,
that this dimension object is measuring.

There are various possibilities:
- An object, and one of its edges.
- An object, and two of its vertices.
- An arc object, and its edge.</translation>
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
      <translation>Punt a través del qual la línia de dimensió, o una extrapolació de la mateixa, passarà.

- Per dimensions linials, aquesta propietat controla com de properes són les línies
de dimensió a l'objecte mesurat.
- Per dimensions radials, controla la direcció de la línia de dimensió que mostra
el radi o diàmetre mesurats.
- Per dimensions angulars, controla el radi de l'arc de dimensió que mostra l'angle mesurat.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="233"/>
      <source>Starting point of the dimension line.

If it is a radius dimension it will be the center of the arc.
If it is a diameter dimension it will be a point that lies on the arc.</source>
      <translation>Punt d'inici d'una línia de cota.

Si mesura un radi, serà el centre de l'arc.
Si mesura un diàmetre, serà el punt que descansa a l'arc.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="247"/>
      <source>Ending point of the dimension line.

If it is a radius or diameter dimension
it will be a point that lies on the arc.</source>
      <translation>Punt final d'una línia de cota.

Si es mesura un radi o un diàmetre, aquest punt és sobre l'arc.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="259"/>
      <source>The direction of the dimension line.
If this remains '(0,0,0)', the direction will be calculated automatically.</source>
      <translation>Direcció de la línia de cota.
Si es manté en '(0,0,0)', la direcció es calcularà automàticament.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="276"/>
      <source>The value of the measurement.

This property is read-only because the value is calculated
from the 'Start' and 'End' properties.

If the 'Linked Geometry' is an arc or circle, this 'Distance'
is the radius or diameter, depending on the 'Diameter' property.</source>
      <translation>Valor de la mesura.

Aquesta propietat és només de lectura, ja que el valor és calculat
des de 'Inici' fins a 'Final'.

Si la 'Geometria enllaçada' és un arc o un cercle, aquesta 'Distància'
és el radi o el diàmetre, segons quina sigui la propietat 'Diàmetre'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="287"/>
      <source>When measuring circular arcs, it determines whether to display
the radius or the diameter value</source>
      <translation>Quan es mesuren arcs cirulars, determina si es mostra el valor
del radi o el diàmetre</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="507"/>
      <source>Starting angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
      <translation>Angle d'inici de la línia de cota (arc circular).
L'arc es dibuixa en sentit contrari a les busques del rellotge.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="518"/>
      <source>Ending angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
      <translation>Angle final de la línia de cota (arc circular).
L'arc es dibuixa en sentit contrari a les busques del rellotge.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="533"/>
      <source>The center point of the dimension line, which is a circular arc.

This is normally the point where two line segments, or their extensions
intersect, resulting in the measured 'Angle' between them.</source>
      <translation>Centre de la línia de cota, que és un arc circular.

Normalment aquest és el punt on intersequen dos segments de línia o
les seves extensions, resultant en l'angle mesurat entre ambdues.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="547"/>
      <source>The value of the measurement.

This property is read-only because the value is calculated from
the 'First Angle' and 'Last Angle' properties.</source>
      <translation>Valor de la mesura.

Aquesta propietat és només de lectura, ja que es calcula a partir
de les propietats 'Primer angle' i 'Últim angle'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="45"/>
      <source>Start angle of the elliptical arc</source>
      <translation>Angle d'inici de l'arc el·líptic</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="49"/>
      <source>End angle of the elliptical arc 

                (for a full circle, give it same value as First Angle)</source>
      <translation>Angle final de l'arc el·líptic (per a una circumferència sencera, introduïu el mateix valor que el primer angle)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="52"/>
      <source>Minor radius of the ellipse</source>
      <translation>Radi menor de l'el·lipse</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="55"/>
      <source>Major radius of the ellipse</source>
      <translation>Radi major de l'el·lipse</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="61"/>
      <source>Area of this object</source>
      <translation>Àrea d'aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/text.py" line="54"/>
      <source>The placement of the base point of the first line</source>
      <translation>Posició del punt base de la primera línia</translation>
    </message>
    <message>
      <location filename="../../draftobjects/text.py" line="66"/>
      <source>The text displayed by this object.
It is a list of strings; each element in the list will be displayed in its own line.</source>
      <translation>Text mostrat per aquest objecte.
Consisteix en una llista de cadenes de text; cada element de la llista es mostrarà en la seva pròpia línia.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="82"/>
      <location filename="../../draftobjects/patharray.py" line="169"/>
      <source>The base object that will be duplicated</source>
      <translation>L'objecte base que es duplicarà</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="192"/>
      <source>List of connected edges in the 'Path Object'.
If these are present, the copies will be created along these subelements only.
Leave this property empty to create copies along the entire 'Path Object'.</source>
      <translation>Llista d'arestes connectades en un 'Objecte de Camí'.
Si són presents, les còpies seran creades tansols al llarg d'aquests subelements. Deixeu la propietat en blanc per crear còpies al llarg de tot l''Objecte de Camí'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="200"/>
      <source>Number of copies to create</source>
      <translation>Nombre de còpies per crear</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="219"/>
      <source>Additional translation that will be applied to each copy.
This is useful to adjust for the difference between shape centre and shape reference point.</source>
      <translation>Traslació addicional que s'aplicarà a cada còpia.
Això és útil d'ajustar per la diferència entre el centre de la forma i el punt de referència de la forma.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="227"/>
      <source>Alignment vector for 'Tangent' mode</source>
      <translation>Vector d'aliniació pel mode 'Tangent'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="235"/>
      <source>Force use of 'Vertical Vector' as local Z direction when using 'Original' or 'Tangent' alignment mode</source>
      <translation>Força l'ús del 'Vector Vertical' com a direcció Z local quan s'utilitza el mode 'Original' o 'Tagent'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="243"/>
      <source>Direction of the local Z axis when 'Force Vertical' is true</source>
      <translation>Direcció de l'eix local Z quan 'Força Vertical' és cert</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="251"/>
      <source>Method to orient the copies along the path.
- Original: X is curve tangent, Y is normal, and Z is the cross product.
- Frenet: aligns the object following the local coordinate system along the path.
- Tangent: similar to 'Original' but the local X axis is pre-aligned to 'Tangent Vector'.

To get better results with 'Original' or 'Tangent' you may have to set 'Force Vertical' to true.</source>
      <translation>Mètode per orientar les còpies al llarg del camí.
- Original: X és tangent a la corba, Y és normal i Z és el producte vectorial.
- Frenet: alinea l'objecte seguint el sistema de coordenades local al llarg del cami.
- Tangent: similar a 'Original' però l'eix X local s'alinea prèviament amb 'Vector Tangent'.

Per obtenir millors resultats amb 'Original' o 'Tangent' proveu d'establir 'Força Vertical' com a cert.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="262"/>
      <source>Orient the copies along the path depending on the 'Align Mode'.
Otherwise the copies will have the same orientation as the original Base object.</source>
      <translation>Orienta les còpies al llarg del camí segons 'Mode d'Aliniació'.
Altrament, les còpies tindran la mateixa orientació que l'objecte base original.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="46"/>
      <source>The points of the Bezier curve</source>
      <translation>Punts de la corba Bezier</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="50"/>
      <source>The degree of the Bezier function</source>
      <translation>Grau de la funció Bezier</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="54"/>
      <source>Continuity</source>
      <translation>Continuitat</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="58"/>
      <source>If the Bezier curve should be closed or not</source>
      <translation>Si la corba de Bézier ha de ser tancada o no</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="62"/>
      <source>Create a face if this curve is closed</source>
      <translation>Crea una cara si aquesta corba és tancada</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="66"/>
      <source>The length of this object</source>
      <translation>Longitud d'aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="98"/>
      <source>The type of array to create.
- Ortho: places the copies in the direction of the global X, Y, Z axes.
- Polar: places the copies along a circular arc, up to a specified angle, and with certain orientation defined by a center and an axis.
- Circular: places the copies in concentric circular layers around the base object.</source>
      <translation>El tipus de matriu per crear.
- Ortho: situa les còpies en la direcció dels eixos globals X, Y, Z
- Polar: situa les còpies sobre un arc de cercle, fins un angle especificat, i amb una certa orientació definida pel centre i un eix.
- Circular: situa les còpies en capes circular concèntriques al voltant d'un objecte base.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="109"/>
      <source>Specifies if the copies should be fused together if they touch each other (slower)</source>
      <translation>Especifica si les còpies s'han de fusionar si es toquen (més lent)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="122"/>
      <source>Number of copies in X direction</source>
      <translation>Nombre de còpies en la direcció X</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="131"/>
      <source>Number of copies in Y direction</source>
      <translation>Nombre de còpies en la direcció Y</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="140"/>
      <source>Number of copies in Z direction</source>
      <translation>Nombre de còpies en la direcció Z</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="150"/>
      <source>Distance and orientation of intervals in X direction</source>
      <translation>Distància i orientació d'intervals en la direcció X</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="160"/>
      <source>Distance and orientation of intervals in Y direction</source>
      <translation>Distància i orientació d'intervals en la direcció Y</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="170"/>
      <source>Distance and orientation of intervals in Z direction</source>
      <translation>Distància i orientació d'intervals en la direcció Z</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="185"/>
      <source>The axis direction around which the elements in a polar or a circular array will be created</source>
      <translation>La direcció de l'eix al voltant del qual es crearan els elements en una matriu polar o circular</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="196"/>
      <source>Center point for polar and circular arrays.
The 'Axis' passes through this point.</source>
      <translation>Punt central per matrius polars i circulars.
L''Eix' passa per aquest punt.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="215"/>
      <source>The axis object that overrides the value of 'Axis' and 'Center', for example, a datum line.
Its placement, position and rotation, will be used when creating polar and circular arrays.
Leave this property empty to be able to set 'Axis' and 'Center' manually.</source>
      <translation>L'objecte de tipus eix que sobreescriu el valor d''Eix' i 'Centre', per exemple una línia de referència.
La col·locació, posició i rotació, s'utilitzaran en crear matrius polars i circulars.
Deixa aquesta propietat buida per poder especificar 'Eix' i 'Centre' manualment.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="228"/>
      <source>Number of copies in the polar direction</source>
      <translation>Nombre de còpies en la direcció polar</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="238"/>
      <source>Distance and orientation of intervals in 'Axis' direction</source>
      <translation>Distància i orientació dels intervals en la direcció 'Eix'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="247"/>
      <source>Angle to cover with copies</source>
      <translation>Angle a cobrir amb còpies</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="260"/>
      <source>Distance between circular layers</source>
      <translation>Distància entre capes circulars</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="270"/>
      <source>Distance between copies in the same circular layer</source>
      <translation>Distància entre còpies dins d'una mateixa capa circular</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="280"/>
      <source>Number of circular layers. The 'Base' object counts as one layer.</source>
      <translation>Nombre de capes circulars. L'objecte 'Base' compta com una capa.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="291"/>
      <source>A parameter that determines how many symmetry planes the circular array will have.</source>
      <translation>Un paràmetre que determina quants plans de simetria tindrà la matriu circular.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="309"/>
      <source>Total number of elements in the array.
This property is read-only, as the number depends on the parameters of the array.</source>
      <translation>Nombre total d'elements dins la matriu.
Aquesta propietat és de només lectura, ja que el nombre depèn dels paràmetres de la matriu.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="79"/>
      <source>Base object that will be duplicated</source>
      <translation>Objecte de base que serà duplicat</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="87"/>
      <source>Object containing points used to distribute the base object, for example, a sketch or a Part compound.
The sketch or compound must contain at least one explicit point or vertex object.</source>
      <translation>L'objecte que conté els punts que s'usaran per distribuir l'objecte de base, per exemple, un esbós o una composició de peces.
L'esbós o la composició han de contenir com a mínim un punt explícit o un objecte vèrtex.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="95"/>
      <source>Total number of elements in the array.
This property is read-only, as the number depends on the points contained within 'Point Object'.</source>
      <translation>Nombre total d'elements a la matriu.
Aquesta propietat és només de lectura, jau que el nombre depèn dels punts continguts dins l''Objecte Punt'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="104"/>
      <location filename="../../draftobjects/pointarray.py" line="140"/>
      <source>Additional placement, shift and rotation, that will be applied to each copy</source>
      <translation>Posicionament, desplaçament o rotació addiciionals que s'aplicarà a cada còpia</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="60"/>
      <location filename="../../draftviewproviders/view_label.py" line="74"/>
      <source>The size of the text</source>
      <translation>Mida del text</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="69"/>
      <location filename="../../draftviewproviders/view_label.py" line="83"/>
      <source>The font of the text</source>
      <translation>Tipus de lletra del text</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="78"/>
      <location filename="../../draftviewproviders/view_label.py" line="92"/>
      <location filename="../../draftviewproviders/view_label.py" line="119"/>
      <source>The vertical alignment of the text</source>
      <translation>Aliniació vertical del text</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="87"/>
      <location filename="../../draftviewproviders/view_label.py" line="102"/>
      <source>Text color</source>
      <translation>Color del text</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="95"/>
      <location filename="../../draftviewproviders/view_label.py" line="128"/>
      <source>Line spacing (relative to font size)</source>
      <translation>Interliniat (relatiu a la mida de la lletra)</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="111"/>
      <source>The maximum number of characters on each line of the text box</source>
      <translation>Nombre màxim de caracters a cada línia d'una caixa de text</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="139"/>
      <source>The size of the arrow</source>
      <translation>Mida de la fletxa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="148"/>
      <source>The type of arrow of this label</source>
      <translation>Tipus de fletxa d'aquesta etiqueta</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="159"/>
      <source>The type of frame around the text of this object</source>
      <translation>Tipus de marc al voltant del text d'aquest objecte</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="168"/>
      <source>Display a leader line or not</source>
      <translation>Mostra o amaga la línia guia</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="115"/>
      <location filename="../../draftviewproviders/view_label.py" line="177"/>
      <source>Line width</source>
      <translation>Amplada de línia</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="122"/>
      <location filename="../../draftviewproviders/view_label.py" line="186"/>
      <source>Line color</source>
      <translation>Color de línia</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="137"/>
      <source>Font name</source>
      <translation>Nom de la font</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="146"/>
      <source>Font size</source>
      <translation>Mida del tipus de lletra</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="155"/>
      <source>Spacing between text and dimension line</source>
      <translation>Espaiat entre el text i la línia de cota</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="164"/>
      <source>Rotate the dimension text 180 degrees</source>
      <translation>Gira el text de la cota 180 graus</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="174"/>
      <source>Text Position.
Leave '(0,0,0)' for automatic position</source>
      <translation>Posició del text.
Deixeu-lo a '(0,0,0)' per a posicionament automàtic</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="185"/>
      <source>Text override.
Write '$dim' so that it is replaced by the dimension length.</source>
      <translation>Text per sobreescriure.
Entreu '$dim' per reemplaçar-lo per la longitud de la cota.</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="196"/>
      <source>The number of decimals to show</source>
      <translation>Nombre de decimals per mostrar</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="205"/>
      <source>Show the unit suffix</source>
      <translation>Mostra el sufix de la unitat</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="216"/>
      <source>A unit to express the measurement.
Leave blank for system default.
Use 'arch' to force US arch notation</source>
      <translation type="unfinished">A unit to express the measurement.
Leave blank for system default.
Use 'arch' to force US arch notation</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="230"/>
      <source>Arrow size</source>
      <translation>Mida de la fletxa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="239"/>
      <source>Arrow type</source>
      <translation>Tipus de fletxa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="249"/>
      <source>Rotate the dimension arrows 180 degrees</source>
      <translation>Gira les fletes d'acotació 180 graus</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="260"/>
      <source>The distance the dimension line is extended
past the extension lines</source>
      <translation>Distància en què les línies de cota s'extendran més enllà de les línies d'extensió</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="269"/>
      <source>Length of the extension lines</source>
      <translation>Longitud de les línies d'extensió</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="279"/>
      <source>Length of the extension line
beyond the dimension line</source>
      <translation>La longitud de la línia d'extensió sobrepassa la línia de cota</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="288"/>
      <source>Shows the dimension line and arrows</source>
      <translation>Mostra la cota i les fletxes</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="67"/>
      <source>If it is true, the objects contained within this layer will adopt the line color of the layer</source>
      <translation>Si és cert, els objectes continguts a aquesta capa adoptarà el color de la línia de la capa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="78"/>
      <source>If it is true, the objects contained within this layer will adopt the shape color of the layer</source>
      <translation type="unfinished">If it is true, the objects contained within this layer will adopt the shape color of the layer</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="89"/>
      <source>If it is true, the print color will be used when objects in this layer are placed on a TechDraw page</source>
      <translation>Si és cert, el color d'impressió serà utilitzat quan els objectes d'aquesta capa siguin posicionats a la pàgina de TechDraw</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="103"/>
      <source>The line color of the objects contained within this layer</source>
      <translation>El color de línia dels objectes continguts dins d'aquesta capa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="117"/>
      <source>The shape color of the objects contained within this layer</source>
      <translation>El color de la forma dels objectes continguts en aquesta capa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="131"/>
      <source>The line width of the objects contained within this layer</source>
      <translation>El gruix de línia dels objectes continguts en aquesta capa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="143"/>
      <source>The draw style of the objects contained within this layer</source>
      <translation>L'estil de dibuix dels objectes continguts en aquesta capa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="154"/>
      <source>The transparency of the objects contained within this layer</source>
      <translation>La transparència dels objectes continguts en aquesta capa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="165"/>
      <source>The line color of the objects contained within this layer, when used on a TechDraw page</source>
      <translation>El color de línia dels objectes continguts en aquesta capa, quan son utilitzats a una pàgina TechDraw</translation>
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
      <translation>Editor d'estils d'anotació</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="20"/>
      <source>Style name</source>
      <translation>Nom de l'estil</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="38"/>
      <source>The name of your style. Existing style names can be edited.</source>
      <translation>El nom del vostre estil. Els noms d'estil existent poden ser editats.</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="50"/>
      <source>Add new...</source>
      <translation>Afegeix nou...</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="79"/>
      <source>Renames the selected style</source>
      <translation>Canvia el nom als elements seleccionats</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="82"/>
      <source>Rename</source>
      <translation>Reanomena</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="110"/>
      <source>Deletes the selected style</source>
      <translation>Esborra l'estil seleccionat</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="113"/>
      <source>Delete</source>
      <translation>Elimina</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="120"/>
      <source>Import styles from json file</source>
      <translation>Importa els estils d'un fitxer JSON</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="130"/>
      <source>Export styles to json file</source>
      <translation>Exporta els estils a un fitxer JSON</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="164"/>
      <source>Text</source>
      <translation>Text</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="170"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="192"/>
      <source>The font to use for texts and dimensions</source>
      <translation>Font a emprar per a textos i dimensions</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="173"/>
      <source>Font name</source>
      <translation>Nom de la font</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="199"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="209"/>
      <source>Font size in the system units</source>
      <translation>Mida de la font en unitats del sistema</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="202"/>
      <source>Font size</source>
      <translation>Mida del tipus de lletra</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="219"/>
      <source>Line spacing in system units</source>
      <translation>Espai entre línies en unitats del sistema</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="222"/>
      <source>Line spacing</source>
      <translation>Interlineat</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="239"/>
      <source>Units</source>
      <translation>Unitats</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="245"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="255"/>
      <source>A multiplier factor that affects the size of texts and markers</source>
      <translation>Un factor multiplicador que afecta la mida del text i marcadors</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="248"/>
      <source>Scale multiplier</source>
      <translation>Multiplicador d'escala</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="268"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="278"/>
      <source>If it is checked it will show the unit next to the dimension value</source>
      <translation>Si està marcada l'opció, mostrarà la unitat al costat del valor de la cota</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="271"/>
      <source>Show unit</source>
      <translation>Mostra les unitats</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="291"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="301"/>
      <source>Specify a valid length unit like mm, m, in, ft, to force displaying the dimension value in this unit</source>
      <translation>Especifiqueu una unitat de longitud vàlida com ara mm, m, in, ft, per forçar que es mostri el valor de l'acotació en aquesta unitat</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="294"/>
      <source>Unit override</source>
      <translation>Substitució d'unitat</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="308"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="318"/>
      <source>The number of decimals to show for dimension values</source>
      <translation>El nombre de decimals pels valors de les acotacions</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="311"/>
      <source>Decimals</source>
      <translation>Decimals</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="331"/>
      <source>Line and arrows</source>
      <translation>Línies i fletxes</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="337"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="347"/>
      <source>If it is checked it will display the dimension line</source>
      <translation>Si està marcada l'opció, es mostrarà la línia de cota</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="340"/>
      <source>Show lines</source>
      <translation>Mostra les línies</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="363"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="373"/>
      <source>The width of the dimension lines</source>
      <translation>L'amplària de les línies d'acotació</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="366"/>
      <source>Line width</source>
      <translation>Amplada de línia</translation>
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
      <translation>El color de les línies d'acotació, fletxes i textos</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="389"/>
      <source>Line / text color</source>
      <translation>Línia / text color</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="410"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="432"/>
      <source>The type of arrows or markers to use at the end of dimension lines</source>
      <translation>Tius de fletxa o marcador que s'emprarà al final de les línies de cota</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="413"/>
      <source>Arrow type</source>
      <translation>Tipus de fletxa</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="436"/>
      <source>Dot</source>
      <translation>Punt</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="441"/>
      <source>Circle</source>
      <translation>Cercle</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="446"/>
      <source>Arrow</source>
      <translation>Fletxa</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="451"/>
      <source>Tick</source>
      <translation>Tick</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="456"/>
      <source>Tick-2</source>
      <translation>Tick-2</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="464"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="474"/>
      <source>The size of the dimension arrows or markers in system units</source>
      <translation>Mida de les fletxes de la cota o marcadors en unitats del sistema</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="467"/>
      <source>Arrow size</source>
      <translation>Mida de la fletxa</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="484"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="494"/>
      <source>The distance that the dimension line is additionally extended</source>
      <translation>Distància addicional que s'extendrà la línia de cota</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="487"/>
      <source>Dimension overshoot</source>
      <translation>Superació de la dimensió</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="504"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="514"/>
      <source>The length of the extension lines</source>
      <translation>Longitud de les línies d'extensió</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="507"/>
      <source>Extension lines</source>
      <translation>Línies d'extensió</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="524"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="534"/>
      <source>The distance that the extension lines are additionally extended beyond the dimension line</source>
      <translation>Distància a la qual les línies d'extensió s'allarguen més enllà de la línia de cota</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="527"/>
      <source>Extension overshoot</source>
      <translation>Superació de l'extensió</translation>
    </message>
  </context>
  <context>
    <name>Draft</name>
    <message>
      <location filename="../../importDXF.py" line="129"/>
      <source>Download of dxf libraries failed.
Please install the dxf Library addon manually
from menu Tools -&gt; Addon Manager</source>
      <translation>Ha fallat la descàrrega de llibreries dxf.
Instal·li el complement de la llibreria dxf 
mitjançant l'opció Eines -&gt;Gestor de complements</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="133"/>
      <location filename="../../InitGui.py" line="134"/>
      <location filename="../../InitGui.py" line="135"/>
      <location filename="../../InitGui.py" line="136"/>
      <location filename="../../InitGui.py" line="137"/>
      <source>Draft</source>
      <translation>Calat</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="179"/>
      <location filename="../../InitGui.py" line="180"/>
      <location filename="../../InitGui.py" line="181"/>
      <location filename="../../InitGui.py" line="182"/>
      <source>Import-Export</source>
      <translation>Importació-exportació</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="248"/>
      <source>Toggles Grid On/Off</source>
      <translation>Conmuta la quadrícula activa/inactiva</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="265"/>
      <source>Object snapping</source>
      <translation>Ajustament d'objectes</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="297"/>
      <source>Toggles Visual Aid Dimensions On/Off</source>
      <translation>Activa o desactiva les cotes d'ajut visual</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="316"/>
      <source>Toggles Ortho On/Off</source>
      <translation>Activa o desactiva l'Ortho</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="334"/>
      <source>Toggles Constrain to Working Plane On/Off</source>
      <translation>Activa o desactiva la restricció al pla de treball</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="106"/>
      <source>_BSpline.createGeometry: Closed with same first/last Point. Geometry not updated.</source>
      <translation>_BSpline.createGeometry: Tancada amb el mateix punt d'inici/final. No es pot actualitzar la geometria.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="244"/>
      <location filename="../../draftobjects/pointarray.py" line="306"/>
      <source>Point object doesn't have a discrete point, it cannot be used for an array.</source>
      <translation>L'objecte de punt no té un punt discret, no es pot usar per a una matriu.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="88"/>
      <location filename="../../draftguitools/gui_lineslope.py" line="91"/>
      <source>Slope</source>
      <translation>Pendent</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="91"/>
      <source>Clone</source>
      <translation>Clona</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="49"/>
      <source>You must choose a base object before using this command</source>
      <translation type="unfinished">You must choose a base object before using this command</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="89"/>
      <source>Delete original objects</source>
      <translation>Esborra els objectes originals</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="95"/>
      <source>Create chamfer</source>
      <translation>Crea un xamfrà</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="274"/>
      <source>Save style</source>
      <translation>Desa l'estil</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="275"/>
      <source>Name of this new style:</source>
      <translation>Nom d'aquest nou estil:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="282"/>
      <source>Warning</source>
      <translation>Avís</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="283"/>
      <source>Name exists. Overwrite?</source>
      <translation>Aquest nom ja existeix. El vols sobreescriure?</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="318"/>
      <source>Error: json module not found. Unable to save style</source>
      <translation>Error: no s'ha trobat el mòdul json. Impossible salvar l'estil</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="329"/>
      <source>Offset direction is not defined. Please move the mouse on either side of the object first to indicate a direction</source>
      <translation>No s'ha definit la direcció de desplaçament. Si us plau, moveu el ratolí a un o altre costat de l'objecte abans per indicar-ne la direcció</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="144"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="156"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="157"/>
      <source>True</source>
      <translation>Cert</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="148"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="160"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="159"/>
      <source>False</source>
      <translation>Fals</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="151"/>
      <source>Scale</source>
      <translation>Escala</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="152"/>
      <source>X factor</source>
      <translation>Factor X</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="153"/>
      <source>Y factor</source>
      <translation>Factor Y</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="154"/>
      <source>Z factor</source>
      <translation>Factor Z</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="155"/>
      <source>Uniform scaling</source>
      <translation>Escalat uniforme</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="156"/>
      <source>Working plane orientation</source>
      <translation>Orientació del pla de treball</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="157"/>
      <source>Copy</source>
      <translation>Copia</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="158"/>
      <source>Modify subelements</source>
      <translation>Modifica els sub-elements</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="159"/>
      <source>Pick from/to points</source>
      <translation>Triar des/fins als punts</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="160"/>
      <source>Create a clone</source>
      <translation>Crear un clon</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_wpproxy.py" line="102"/>
      <source>Writing camera position</source>
      <translation>Escrivint la posició de la càmera</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_wpproxy.py" line="119"/>
      <source>Writing objects shown/hidden state</source>
      <translation>Escrivint l'estat visible/ocult dels objectes</translation>
    </message>
  </context>
  <context>
    <name>DraftCircularArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="26"/>
      <source>Circular array</source>
      <translation>Matriu circular</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(Espai reservat per a la icona)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="53"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="63"/>
      <source>Distance from one layer of objects to the next layer of objects.</source>
      <translation>Distància d'una capa dels objectes a la capa d'objectes següent.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="56"/>
      <source>Radial distance</source>
      <translation>Distància radial</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="76"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="87"/>
      <source>Distance from one element in one ring of the array to the next element in the same ring.
It cannot be zero.</source>
      <translation>Distància d'un element en un anell de la matriu a l'element següent del mateix anell.
No pot ser zero.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="80"/>
      <source>Tangential distance</source>
      <translation>Distància tangencial</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="101"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="112"/>
      <source>Number of circular layers or rings to create, including a copy of the original object.
It must be at least 2.</source>
      <translation>Nombre de capes circulars o anells que s'han de crear, inclosa una còpia de l'objecte original.
Ha de ser pel cap baix 2.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="105"/>
      <source>Number of circular layers</source>
      <translation>Nombre de capes circulars</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="126"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="136"/>
      <source>The number of symmetry lines in the circular array.</source>
      <translation>El nombre de línies de simetria de la matriu circular.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="129"/>
      <source>Symmetry</source>
      <translation>Simetria</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="151"/>
      <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
      <translation>Les coordenades del punt per on passa l’eix de rotació.
Canvieu la direcció del propi eix a l’editor de propietats.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="155"/>
      <source>Center of rotation</source>
      <translation>Centre de rotació</translation>
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
      <translation>Reinicia les coordenades del centre de rotació.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="228"/>
      <source>Reset point</source>
      <translation>Reinicia el punt</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="240"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Si està marcada, els objectes que resulten en la matriu es fusionaran si es toquen.
Això només funciona si "Matriu d'enllaç" està desactivada.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="244"/>
      <source>Fuse</source>
      <translation>Combina</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="251"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Si està marcada, l'objecte que en resulte serà una "matriu d'enllaç" en lloc d'una matriu regular.
Una matriu d'enllaç és més eficient quan es creen diverses còpies, però no es pot fusionar.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="255"/>
      <source>Link array</source>
      <translation>Matriu d'enllaç</translation>
    </message>
  </context>
  <context>
    <name>DraftOrthoArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="26"/>
      <source>Orthogonal array</source>
      <translation>Matriu ortogonal</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(Espai reservat per a la icona)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="51"/>
      <source>Number of elements in the array in the specified direction, including a copy of the original object.
The number must be at least 1 in each direction.</source>
      <translation>Nombre d'elements de la matriu en la direcció especificada, inclosa una còpia de l'objecte original.
El nombre ha de ser com a mínim 1 en cada direcció.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="55"/>
      <source>Number of elements</source>
      <translation>Nombre d'elements</translation>
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
      <translation>Distància entre els elements en la direcció X.
Normalment, només és necessari el valor X; els altres dos valors poden possibilitar un canvi addicional en les seves direccions respectives.
Els valors negatius donaran lloc a còpies produïdes en la direcció negativa.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="124"/>
      <source>X intervals</source>
      <translation>Intervals en X</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="197"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="288"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="379"/>
      <source>Reset the distances.</source>
      <translation>Tornar a establir les distàncies.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="200"/>
      <source>Reset X</source>
      <translation>Reinicia X</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="210"/>
      <source>Distance between the elements in the Y direction.
Normally, only the Y value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Distància entre els elements en direcció Y.
Normalment, només és necessari el valor Y; els altres dos valors poden possibilitar un canvi addicional en les seves direccions respectives.
Els valors negatius donaran lloc a còpies produïdes en la direcció negativa.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="215"/>
      <source>Y intervals</source>
      <translation>Intervals en Y</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="291"/>
      <source>Reset Y</source>
      <translation>Reinicia Y</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="301"/>
      <source>Distance between the elements in the Z direction.
Normally, only the Z value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Distància entre els elements en direcció Z.
Normalment, només és necessari el valor Z; els altres dos valors poden possibilitar un canvi addicional en les seves direccions respectives.
Els valors negatius donaran lloc a còpies produïdes en la direcció negativa.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="306"/>
      <source>Z intervals</source>
      <translation>Intervals en Z</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="382"/>
      <source>Reset Z</source>
      <translation>Reinicia Z</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="394"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Si està marcada, els objectes que resulten en la matriu es fusionaran si es toquen.
Això només funciona si "Matriu d'enllaç" està desactivada.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="398"/>
      <source>Fuse</source>
      <translation>Combina</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="405"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Si està marcada, l'objecte que en resulte serà una "matriu d'enllaç" en lloc d'una matriu regular.
Una matriu d'enllaç és més eficient quan es creen diverses còpies, però no es pot fusionar.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="409"/>
      <source>Link array</source>
      <translation>Matriu d'enllaç</translation>
    </message>
  </context>
  <context>
    <name>DraftPolarArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="26"/>
      <source>Polar array</source>
      <translation>Matriu polar</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(Espai reservat per a la icona)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="53"/>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="65"/>
      <source>Sweeping angle of the polar distribution.
A negative angle produces a polar pattern in the opposite direction.
The maximum absolute value is 360 degrees.</source>
      <translation>Angle d'escombratge de la distribució polar.
Un angle negatiu produeix un patró polar en la direcció oposada.
El valor màxim absolut és de 360 graus.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="58"/>
      <source>Polar angle</source>
      <translation>Angle polar</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="86"/>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="97"/>
      <source>Number of elements in the array, including a copy of the original object.
It must be at least 2.</source>
      <translation>Nombre d'elements de la matriu, inclosa una còpia de l'objecte original. Ha de ser almenys 2.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="90"/>
      <source>Number of elements</source>
      <translation>Nombre d'elements</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="113"/>
      <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
      <translation>Les coordenades del punt per on passa l’eix de rotació.
Canvieu la direcció del propi eix a l’editor de propietats.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="117"/>
      <source>Center of rotation</source>
      <translation>Centre de rotació</translation>
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
      <translation>Reinicia les coordenades del centre de rotació.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="190"/>
      <source>Reset point</source>
      <translation>Reinicia el punt</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="202"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Si està marcada, els objectes que resulten en la matriu es fusionaran si es toquen.
Això només funciona si "Matriu d'enllaç" està desactivada.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="206"/>
      <source>Fuse</source>
      <translation>Combina</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="213"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Si està marcada, l'objecte que en resulte serà una "matriu d'enllaç" en lloc d'una matriu regular.
Una matriu d'enllaç és més eficient quan es creen diverses còpies, però no es pot fusionar.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="217"/>
      <source>Link array</source>
      <translation>Matriu d'enllaç</translation>
    </message>
  </context>
  <context>
    <name>DraftShapeStringGui</name>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="26"/>
      <source>ShapeString</source>
      <translation>Forma d'Etiqueta</translation>
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
      <translation>Entrar coordenades o selecionar un punt amb el ratolì.</translation>
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
      <translation>Redefinir selecció de punts 3d</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="120"/>
      <source>Reset Point</source>
      <translation>Reiniciar Punt</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="131"/>
      <source>String</source>
      <translation>Cadena</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="138"/>
      <source>Text to be made into ShapeString</source>
      <translation>Text per a convertir en TextForma</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="149"/>
      <source>Height</source>
      <translation>Alçària</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="156"/>
      <source>Height of the result</source>
      <translation>Alçada del resultat</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="176"/>
      <source>Font file</source>
      <translation>Fitxer de tipus de lletra</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddConstruction</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="309"/>
      <source>Add to Construction group</source>
      <translation>Afegir al grup de construcció</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="312"/>
      <source>Adds the selected objects to the construction group,
and changes their appearance to the construction style.
It creates a construction group if it doesn't exist.</source>
      <translation>Afegeix els objectes seleccionats al grup de construcció, i canvia la seva aparença a l'estil de construcció.
Crea un grup de construcció si aquest no existeix.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddNamedGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="361"/>
      <source>Add a new named group</source>
      <translation>Afegeix un grup amb nom nou</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="365"/>
      <source>Add a new group with a given name.</source>
      <translation type="unfinished">Add a new group with a given name.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddPoint</name>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="57"/>
      <source>Add point</source>
      <translation>Afegir un punt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="60"/>
      <source>Adds a point to an existing Wire or B-spline.</source>
      <translation>Afegeix un punt a un filferro a B-spline existent.</translation>
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
      <translation>Mou els objectes seleccionats a un grup existent, o els elimina de qualsevol grup.
El primer cop que s'utilitza crea un grup.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AnnotationStyleEditor</name>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="78"/>
      <source>Annotation styles...</source>
      <translation>Estils de les anotacions...</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="82"/>
      <source>Manage or create annotation styles</source>
      <translation>Gestiona o crea estils d’anotació</translation>
    </message>
  </context>
  <context>
    <name>Draft_ApplyStyle</name>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="47"/>
      <source>Apply current style</source>
      <translation>Aplica l'estil actual</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="50"/>
      <source>Applies the current style defined in the toolbar (line width and colors) to the selected objects and groups.</source>
      <translation>Aplica l'estil definit actualment a la barra d'eines (ample de línia i colors) als objectes i grups seleccionats.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Arc</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="66"/>
      <source>Arc</source>
      <translation>Arc</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="69"/>
      <source>Creates a circular arc by a center point and a radius.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Crea un arc de circumferència a partir del seu centre i un radi.
CTRL per ajustar, MAJÚS per restringir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ArcTools</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="606"/>
      <source>Arc tools</source>
      <translation>Eines d'arc</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="609"/>
      <source>Create various types of circular arcs.</source>
      <translation>Crea diversos tipus d'arcs de circumferència.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Arc_3Points</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="487"/>
      <source>Arc by 3 points</source>
      <translation>Arc per 3 punts</translation>
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
      <translation>Matriu, Array</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="71"/>
      <source>Creates an array from a selected object.
By default, it is a 2x2 orthogonal array.
Once the array is created its type can be changed
to polar or circular, and its properties can be modified.</source>
      <translation>Crea una matriu a partir de l'objecte seleccionat.
Per defecte crea una matriu ortogonal de 2x2.
Un cop s'ha creat la matriu, se'n pot canviar el tipus a
polar o circular, i se'n poden modificar les propietats.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ArrayTools</name>
    <message>
      <location filename="../../draftguitools/gui_arrays.py" line="65"/>
      <source>Array tools</source>
      <translation>Eines de taules</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arrays.py" line="68"/>
      <source>Create various types of arrays, including rectangular, polar, circular, path, and point</source>
      <translation>Crea diversos tipus de matriu, incloent rectangular, polar, circular, camí i punt</translation>
    </message>
  </context>
  <context>
    <name>Draft_AutoGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="208"/>
      <source>Autogroup</source>
      <translation>Agrupament automàtic</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="211"/>
      <source>Select a group to add all Draft and Arch objects to.</source>
      <translation>Selecciona un grup on s'afegiran tots els objectes Esbós i Arquitectura.</translation>
    </message>
  </context>
  <context>
    <name>Draft_BSpline</name>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="60"/>
      <source>B-spline</source>
      <translation>B-spline</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="63"/>
      <source>Creates a multiple-point B-spline. CTRL to snap, SHIFT to constrain.</source>
      <translation>Crea una B-spline de múltiples punts. CTRL per ajustar, MAJÚS per restringir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_BezCurve</name>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="64"/>
      <source>Bézier curve</source>
      <translation>Corba de Bézier</translation>
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
      <translation>Eines per a corbes Bézier</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="478"/>
      <source>Create various types of Bézier curves.</source>
      <translation>Crea diversos tipus de corbes de Bézier.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Circle</name>
    <message>
      <location filename="../../draftguitools/gui_circles.py" line="80"/>
      <source>Circle</source>
      <translation>Cercle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_circles.py" line="84"/>
      <source>Creates a circle (full circular arc).
CTRL to snap, ALT to select tangent objects.</source>
      <translation>Crea una circumferència (arc circular sencer).
CTRL per ajustar, ALT per seleccionar objectes tangents.</translation>
    </message>
  </context>
  <context>
    <name>Draft_CircularArray</name>
    <message>
      <location filename="../../draftguitools/gui_circulararray.py" line="65"/>
      <source>Circular array</source>
      <translation>Matriu circular</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_circulararray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in a radial pattern
creating various circular layers.

The array can be turned into an orthogonal or a polar array by changing its type.</source>
      <translation>Crea còpies de l'objecte seleccionat i els disposa radialment, creand diverses capes circulars.

La matriu es pot transformar en ortogonal o polar canviant-ne el tipus.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Clone</name>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="70"/>
      <source>Clone</source>
      <translation>Clona</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="73"/>
      <source>Creates a clone of the selected objects.
The resulting clone can be scaled in each of its three directions.</source>
      <translation>Crea un clon dels objectes selecctionats.
El clon resultant pot ésser escalat en qualsevol de les seves tres direccions.</translation>
    </message>
  </context>
  <context>
    <name>Draft_CubicBezCurve</name>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="242"/>
      <source>Cubic Bézier curve</source>
      <translation>Corba de Bézier cúbica</translation>
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
      <translation>Eliminar punt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="92"/>
      <source>Removes a point from an existing Wire or B-spline.</source>
      <translation>Elimina un punt d'un filferrro o B-spline existent.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Dimension</name>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="84"/>
      <source>Dimension</source>
      <translation>Cota</translation>
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
      <translation>Crea una acotació.

- Trieu tres punts per crear una acotació linial simple.
- Trieu una línia recta per crear una acotació linial lligada a la recta.
- Trieu un arc o una circumferència per crear una acotació de radi o diàmetre associada a aquest arc.
- Seleccioneu dues línies rectes per crear una acotació angular entre elles.
CTRL per ajustar, MAJÚS per restringir, ALT per tiar una aresta o un arc.

Podeu seleccionar una sola linia o un sol arc circular abans d'invocar aquesta ordre per convertir-lo en un objecte 'Esbós d'acotació'.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Downgrade</name>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="58"/>
      <source>Downgrade</source>
      <translation>Baixar de categoria</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="61"/>
      <source>Downgrades the selected objects into simpler shapes.
The result of the operation depends on the types of objects, which may be able to be downgraded several times in a row.
For example, it explodes the selected polylines into simpler faces, wires, and then edges. It can also subtract faces.</source>
      <translation>Degrada els objectes seleccionats a les seves formes més simples.
El resultat de l'operació depèn dels tipus d'objectes, que poden ésser degradats diverses vegades.
Per exemple, explota les polilínies seleccionades e cares més simples, filferros i després arestes. També pot restar cares.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Draft2Sketch</name>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="57"/>
      <source>Draft to Sketch</source>
      <translation>Converteix un Esbós en Croquis</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="60"/>
      <source>Convert bidirectionally between Draft objects and Sketches.
Many Draft objects will be converted into a single non-constrained Sketch.
However, a single sketch with disconnected traces will be converted into several individual Draft objects.</source>
      <translation>Conversió bidireccional entre objectes Esbós i Croquis.
Diversos objectes Esbós es convertiran en un sol Croquis sense restriccions, mentre que un sol Croquis amb traços no continus es convertirà en diversos objectes Esbós individuals.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Drawing</name>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="71"/>
      <source>Drawing</source>
      <translation>Dibuixant</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="74"/>
      <source>Creates a 2D projection on a Drawing Workbench page from the selected objects.
This command is OBSOLETE since the Drawing Workbench became obsolete in 0.17.
Use TechDraw Workbench instead for generating technical drawings.</source>
      <translation>Crea una projecció 2D en una pàgina del banc de treball de dibuix a partur dels objectes seleccionats.
Aquesta ordre és OBSOLETA, ja que el banc de treball de Dibuix es va convertir en obsolet a la versió 0.17. Useu el banc de treball Dibuix Tècnic per a fer dibuixos tècnics.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Edit</name>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="275"/>
      <source>Edit</source>
      <translation>Edita</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="284"/>
      <source>Edits the active object.
Press E or ALT+LeftClick to display context menu
on supported nodes and on supported objects.</source>
      <translation>Edita l'objecte actiu.
Premeu E o bé ALT+botó dret per mostrar un menú contextual en els nodes suportats i en els objectes suportats.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Ellipse</name>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="59"/>
      <source>Ellipse</source>
      <translation>El·lipse</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="62"/>
      <source>Creates an ellipse. CTRL to snap.</source>
      <translation>Crea una el·lipse. CTRL per ajustar.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Facebinder</name>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="60"/>
      <source>Facebinder</source>
      <translation>Ajunta-cares</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="62"/>
      <source>Creates a facebinder object from selected faces.</source>
      <translation>Crea un objecte Lligacares amb les cares seleccionades.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Fillet</name>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="64"/>
      <source>Fillet</source>
      <translation>Arrodoniment</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="67"/>
      <source>Creates a fillet between two selected wires or edges.</source>
      <translation>Crea un cantell entre dos filferros o arestes seleccionades.</translation>
    </message>
  </context>
  <context>
    <name>Draft_FlipDimension</name>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="62"/>
      <source>Flip dimension</source>
      <translation>Inverteix l'acotació</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="66"/>
      <source>Flip the normal direction of the selected dimensions (linear, radial, angular).
If other objects are selected they are ignored.</source>
      <translation>Inverteix la direcció normal de l'acotació seleccionada (linial, radial, angular).
Si hi ha altres objectes seleccionats seran ignorats.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Hatch</name>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="38"/>
      <source>Hatch</source>
      <translation>Trama</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="42"/>
      <source>Creates hatches on the faces of a selected object</source>
      <translation type="unfinished">Creates hatches on the faces of a selected object</translation>
    </message>
  </context>
  <context>
    <name>Draft_Heal</name>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="58"/>
      <source>Heal</source>
      <translation>Reparar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="61"/>
      <source>Heal faulty Draft objects saved with an earlier version of the program.
If an object is selected it will try to heal that object in particular,
otherwise it will try to heal all objects in the active document.</source>
      <translation>Repara objectes Esbós malmesos desats amb una versió anterior del programa.
Si hi ha un objecte seleccionat, intentarà reparar l'objecte en qüestió, altrament intentarà reparar tots els objectes del document actiu.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Join</name>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="66"/>
      <source>Join</source>
      <translation>Unió</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="69"/>
      <source>Joins the selected lines or polylines into a single object.
The lines must share a common point at the start or at the end for the operation to succeed.</source>
      <translation>Uneix les línies o polilínies seleccionades en un sol objecte.
Les línies han de compartir un punt al principi o al final perquè l'operació tingui èxit.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Label</name>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="64"/>
      <source>Label</source>
      <translation>Etiqueta</translation>
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
      <translation>Crea una etiqueta, que pot estar vinculada a un objecte o subelement seleccionat.

Primer trieu el vèrtex, aresta o cara d'un objecte, aleshores crideu aquesta ordre, després definiu la posició de la línia guia i l'etiqueta textual.
L'etiqueta podrà mostrar informació sobre aquest objecte i sobre el subelement seleccionat, si n'hi ha cap.

Si hi ha molts objectes o subelement seleccionats, només es mostrarà informació del primer en cada cas.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Layer</name>
    <message>
      <location filename="../../draftguitools/gui_layers.py" line="52"/>
      <source>Layer</source>
      <translation>Capa</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_layers.py" line="55"/>
      <source>Adds a layer to the document.
Objects added to this layer can share the same visual properties such as line color, line width, and shape color.</source>
      <translation>Afegeix una capa al document.
Els objectes afegits a aquesta capa poden compartir les mateixes propietats com ara color de línia, ample de línia i color de la forma.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Line</name>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="63"/>
      <source>Line</source>
      <translation>Línia</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="66"/>
      <source>Creates a 2-point line. CTRL to snap, SHIFT to constrain.</source>
      <translation>Crea una línia de 2 punts. CTRL per ajustar, MAJÚS per restringir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_LinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="119"/>
      <source>LinkArray</source>
      <translation>Matriu d'enllaç</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="122"/>
      <source>Like the Array tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>Igual que l'eina de Matriu, però en aquest cas crea una Matriu d'enllaç.
Una 'Matriu d'enllaç' és més eficient quan s'empren moltes còpies, però l'opció 'Fusiona' no es pot utilitzar.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Mirror</name>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="63"/>
      <source>Mirror</source>
      <translation>Reflexa</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="66"/>
      <source>Mirrors the selected objects along a line defined by two points.</source>
      <translation>Reflexa els objectes seleccionats a partir d'una línia definida per dos punts.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Move</name>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="63"/>
      <source>Move</source>
      <translation>Mou</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="66"/>
      <source>Moves the selected objects from one base point to another point.
If the "copy" option is active, it will create displaced copies.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Mou els objectes seleccionats d'un punt base a un altre punt.
Si l'opció "copia" està activada, crearà còpies desplaçades.
CTRL per ajustar, MAJÚS per restringir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Offset</name>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="63"/>
      <source>Offset</source>
      <translation>Equidistancia (ofset)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="66"/>
      <source>Offsets of the selected object.
It can also create an offset copy of the original object.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
      <translation>Desplaçaments de l'objecte seleccionat.
També pit crear una còpia desplaçada de l'objecte original.
CTRL per ajustar, MAJÚS per restringir. Manteniu ALT i feu clic per crear una còpia per cada clic.</translation>
    </message>
  </context>
  <context>
    <name>Draft_OrthoArray</name>
    <message>
      <location filename="../../draftguitools/gui_orthoarray.py" line="65"/>
      <source>Array</source>
      <translation>Matriu, Array</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_orthoarray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in an orthogonal pattern,
meaning the copies follow the specified direction in the X, Y, Z axes.

The array can be turned into a polar or a circular array by changing its type.</source>
      <translation>Crea còpies de l'objecte seleccionat i les disposa en un patró ortogonal, fent que les còpies segueixin la direcció definida en els eixos X, Y i Z.

La matriu es pot convertir en polar o circular canviant-ne el tipus.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathArray</name>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="73"/>
      <source>Path array</source>
      <translation>Matriu de camí</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="76"/>
      <source>Creates copies of the selected object along a selected path.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
      <translation>Crea còpies de l'objecte seleccionat al llarg d'un camí determinat.
Primer trieu l'objecte, i després trieu el camí.
El camí pot ser una polilínia, B-spline, corba Bézier, o fins i tot, arestes d'altres objectes.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="163"/>
      <source>Path Link array</source>
      <translation>Matriu enllaçada de camí</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="166"/>
      <source>Like the PathArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>Igual que l'eina Matriu de camí, però crea una matriu d'enllaços.
Una Matriu d'enllaços és més eficient gestionant moltes còpies, però l'opció 'fusiona' no es pot emprar.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathTwistedArray</name>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="66"/>
      <source>Path twisted array</source>
      <translation>Matriu de camí retorçat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="69"/>
      <source>Creates copies of the selected object along a selected path, and twists the copies.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
      <translation>Crea còpies de l'objecte seleccionat al llarg d'un camí determinat, i en gira les còpies.
Primer trieu l'objecte, i després trieu el camí.
El camí pot ser una polilínia, B-spline, corba Bézier, o fins i tot, arestes d'altres objectes.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathTwistedLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="121"/>
      <source>Path twisted Link array</source>
      <translation>Matriu d'enllaços de camí retorçat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="124"/>
      <source>Like the PathTwistedArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>Igual que l'eina Matriu de camí retorçat, però crea una matriu d'enllaços.
Una Matriu d'enllaços és més eficient gestionant moltes còpies, però l'opció 'fusiona' no es pot emprar.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Point</name>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="62"/>
      <source>Point</source>
      <translation>Punt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="65"/>
      <source>Creates a point object. Click anywhere on the 3D view.</source>
      <translation>Crea un objecte de punt. Cliqueu en qualsevol punt de la vista 3D.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PointArray</name>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="76"/>
      <source>Point array</source>
      <translation>Matriu de punts</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="79"/>
      <source>Creates copies of the selected object, and places the copies at the position of various points.

The points need to be grouped under a compound of points before using this tool.
To create this compound, select various points and then use the Part Compound tool,
or use the Draft Upgrade tool to create a 'Block', or create a Sketch and add simple points to it.

Select the base object, and then select the compound or the sketch to create the point array.</source>
      <translation>Crea còpies de l'objecte seleccionat i en situa les còpies a la posició de diversos punts.

Els punts han d'estar agrupats en una composició de punts abans d'usar aquesta eina.
Per crear aquesta composició, trieu diversos punts i utilitzeu l'eina Composició de peces, o bé useu l'eina Promoció d'esbós per crear un «Bloc», o creeu un croquis i afegiu-hi punts simples.
Seleccioneu l'objecte de base i després la composició o croquis per crear la matriu de punts.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PointLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="141"/>
      <source>PointLinkArray</source>
      <translation>Matriu d'enllaços de punts</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="144"/>
      <source>Like the PointArray tool, but creates a 'Point link array' instead.
A 'Point link array' is more efficient when handling many copies.</source>
      <translation>Com l'eina Matriu de punts, però crea una 'Matriu d'enllaços de punts'.
Una 'Matriu d'enllaços de punts' és més eficient quan s'empren moltes còpies.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PolarArray</name>
    <message>
      <location filename="../../draftguitools/gui_polararray.py" line="65"/>
      <source>Polar array</source>
      <translation>Matriu polar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polararray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in a polar pattern
defined by a center of rotation and its angle.

The array can be turned into an orthogonal or a circular array by changing its type.</source>
      <translation>Crea còpies de l'objecte seleccionat, i disposa les còpies en un patró polar definit per n centre i una rotació del seu angle.

La matriu es pot convertir en ortogonal o circular canviant-ne el tipus.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Polygon</name>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="57"/>
      <source>Polygon</source>
      <translation>Polígon</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="60"/>
      <source>Creates a regular polygon (triangle, square, pentagon, ...), by defining the number of sides and the circumscribed radius.
CTRL to snap, SHIFT to constrain</source>
      <translation>Crea un polígon regular (triangle, quadrat, pentàgon,...), definit pel nombre de cares i el radi circumscrit.
CTRL per ajustar, MAJÚS per restringir</translation>
    </message>
  </context>
  <context>
    <name>Draft_Rectangle</name>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="54"/>
      <source>Rectangle</source>
      <translation>Rectangle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="57"/>
      <source>Creates a 2-point rectangle. CTRL to snap.</source>
      <translation>Crea un rectangle amb 2 punts. CTRL per ajustar.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Rotate</name>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="62"/>
      <source>Rotate</source>
      <translation>Rotació</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="65"/>
      <source>Rotates the selected objects. Choose the center of rotation, then the initial angle, and then the final angle.
If the "copy" option is active, it will create rotated copies.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
      <translation>Fa rotar els objectes seleccionats. Trieu el centre de rotació, l'angle inicial i l'angle final.
Si s'ha activat l'opció "còpia", crearà còpies rotades.
CTRL per ajustar, MAJÚS per restringir. Mantingueu premut ALT i cliqueu per crear una còpia a cada clic.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Scale</name>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="71"/>
      <source>Scale</source>
      <translation>Escala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="74"/>
      <source>Scales the selected objects from a base point.
CTRL to snap, SHIFT to constrain, ALT to copy.</source>
      <translation>Escala els objectes seleccionats a partir d'un punt base.
CTRL per ajustar, MAJÚS per restringir, ALT per a copiar.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SelectGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="164"/>
      <source>Select group</source>
      <translation>Selecciona grup</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="167"/>
      <source>Selects the contents of selected groups. For selected non-group objects, the contents of the group they are in is selected.</source>
      <translation type="unfinished">Selects the contents of selected groups. For selected non-group objects, the contents of the group they are in is selected.</translation>
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
      <translation>Selecciona una cara d'un cos sòlid per crear un pla de treball sobre el qual dibuixar objectes esbós.
Podeu seleccionar també tres vèrtexs o un «proxy» de pla de preball.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SetStyle</name>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="55"/>
      <source>Set style</source>
      <translation>Defineix estil</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="57"/>
      <source>Sets default styles</source>
      <translation>Defineix estils per defecte</translation>
    </message>
  </context>
  <context>
    <name>Draft_Shape2DView</name>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="59"/>
      <source>Shape 2D view</source>
      <translation>Vista 2D de la forma</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="62"/>
      <source>Creates a 2D projection of the selected objects on the XY plane.
The initial projection direction is the negative of the current active view direction.
You can select individual faces to project, or the entire solid, and also include hidden lines.
These projections can be used to create technical drawings with the TechDraw Workbench.</source>
      <translation>Crea una projecció 2D dels objectes seleccionats sobre el pla XY.
La direcció de projecció inicial és la negativa de la direcció de la vista actual.
Podeu triar cares individuals per ser projectades, o bé tot el sòlid, i incloure també línies ocultes.
Aquestes projeccions poden usar-se per crear dibuixos tècnics amb el banc de treball Dibuix Tècnic.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ShapeString</name>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="66"/>
      <source>Shape from text</source>
      <translation>Forma a partir de text</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="68"/>
      <source>Creates a shape from a text string by choosing a specific font and a placement.
The closed shapes can be used for extrusions and boolean operations.</source>
      <translation>Crea una forma a partir d'una cadena de text triant una tipografia i una situació.
Les formes tancades es poden emprar per a extrusions i per a operacions booleanes.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ShowSnapBar</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="589"/>
      <source>Show snap toolbar</source>
      <translation>Mostra la barra d'eines d'ajustar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="592"/>
      <source>Show the snap toolbar if it is hidden.</source>
      <translation>Mostra la barra d'eines d'ajustar is aquesta es troba oculta.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Slope</name>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="70"/>
      <source>Set slope</source>
      <translation>Estableix la pendent</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="73"/>
      <source>Sets the slope of the selected line by changing the value of the Z value of one of its points.
If a polyline is selected, it will apply the slope transformation to each of its segments.

The slope will always change the Z value, therefore this command only works well for
straight Draft lines that are drawn in the XY plane. Selected objects that aren't single lines will be ignored.</source>
      <translation>Estableix la pendent de la línia seleccionada canviant el valor de Z d'un dels seus punts.
Si es tria una polilínia, s'aplicarà la transformació de pendent a cada un dels seus segments.

Pendent sempre canviarà el valor de Z, així que aquesta ordre només funciona bé per línies rectes dibuixades sobre el pla XY. Els objectes seleccionats que no són simples línies seran ignorats.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Angle</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="344"/>
      <source>Angle</source>
      <translation>Angle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="347"/>
      <source>Set snapping to points in a circular arc located at multiples of 30 and 45 degree angles.</source>
      <translation>Ajusta a punts d'un arc circular situats a angles múltiples de 30 i 45 graus.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Center</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="374"/>
      <source>Center</source>
      <translation>Centre</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="377"/>
      <source>Set snapping to the center of a circular arc.</source>
      <translation>Ajusta al centre d'un arc circular.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Dimensions</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="526"/>
      <source>Show dimensions</source>
      <translation>Mostra les acotacions</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="529"/>
      <source>Show temporary linear dimensions when editing an object and using other snapping methods.</source>
      <translation>Mostra les cotes linials temporals quan s'edita un objecte usant altres mètodes d'ajustament.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Endpoint</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="313"/>
      <source>Endpoint</source>
      <translation>Punt final</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="316"/>
      <source>Set snapping to endpoints of an edge.</source>
      <translation>Ajusta als extrems d'una aresta.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Extension</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="404"/>
      <source>Extension</source>
      <translation>Extensió</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="407"/>
      <source>Set snapping to the extension of an edge.</source>
      <translation>Ajusta a l'extensió d'una aresta.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Grid</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="223"/>
      <source>Grid</source>
      <translation>Quadrícula</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="226"/>
      <source>Set snapping to the intersection of grid lines.</source>
      <translation>Ajusta a la intersecció de les línies de la graella.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Intersection</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="253"/>
      <source>Intersection</source>
      <translation>Intersecció</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="256"/>
      <source>Set snapping to the intersection of edges.</source>
      <translation>Ajusta a la intersecció d'arestes.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Lock</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="133"/>
      <source>Main snapping toggle On/Off</source>
      <translation>Activa/Desactiva l'ajustament global</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="136"/>
      <source>Activates or deactivates all snap methods at once.</source>
      <translation>Activa o desactiva tots els mètodes d'ajustament d'un sol cop.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Midpoint</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="163"/>
      <source>Midpoint</source>
      <translation>Punt mig</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="166"/>
      <source>Set snapping to the midpoint of an edge.</source>
      <translation>Ajusta al punt mig d'una aresta.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Near</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="434"/>
      <source>Nearest</source>
      <translation>El més proper</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="437"/>
      <source>Set snapping to the nearest point of an edge.</source>
      <translation>Ajusta al punt més proper d'una aresta.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Ortho</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="465"/>
      <source>Orthogonal</source>
      <translation>Ortogonal</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="468"/>
      <source>Set snapping to a direction that is a multiple of 45 degrees from a point.</source>
      <translation>Ajusta a una direcció que és múltiple de 45 graus des d'un punt.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Parallel</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="283"/>
      <source>Parallel</source>
      <translation>Paral·lel</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="286"/>
      <source>Set snapping to a direction that is parallel to an edge.</source>
      <translation>Ajusta a una direcció paral·lela a una aresta.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Perpendicular</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="193"/>
      <source>Perpendicular</source>
      <translation>Perpendicular</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="196"/>
      <source>Set snapping to a direction that is perpendicular to an edge.</source>
      <translation>Ajusta a una direcció perpendicular a una aresta.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Special</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="495"/>
      <source>Special</source>
      <translation>Especial</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="498"/>
      <source>Set snapping to the special points defined inside an object.</source>
      <translation>Ajusta als punts especials definits dins d'un objecte.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_WorkingPlane</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="559"/>
      <source>Working plane</source>
      <translation>Pla de treball</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="562"/>
      <source>Restricts snapping to a point in the current working plane.
If you select a point outside the working plane, for example, by using other snapping methods,
it will snap to that point's projection in the current working plane.</source>
      <translation>Restringeix l'ajustament a un punt dins del pla de treball actual.
Si trieu un punt fora del pla de treball, per exemple, emprant altres mètodes d'ajustament, s'ajustarà a la projecció del punt en el pla de treball actual.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Split</name>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="56"/>
      <source>Split</source>
      <translation>Dividir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="59"/>
      <source>Splits the selected line or polyline into two independent lines
or polylines by clicking anywhere along the original object.
It works best when choosing a point on a straight segment and not a corner vertex.</source>
      <translation>Divideix la línia o polilínia seleccionada en dos línies independents o polilínies, fent clic a qualsevol lloc al llarg de l'objecte original.
Funciona millor quan es tria un punt d'un segment recte i no en un vèrtex a la cantonada.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Stretch</name>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="64"/>
      <source>Stretch</source>
      <translation>Estira</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="67"/>
      <source>Stretches the selected objects.
Select an object, then draw a rectangle to pick the vertices that will be stretched,
then draw a line to specify the distance and direction of stretching.</source>
      <translation>Estira els objectes seleccionats.
Seleccioneu un objecte, dibuixeu un rectangle per triar els vèrtexs que seran estirats, i finalment dibuixeu una línia per definir la distància i direcció d'estirament.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SubelementHighlight</name>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="61"/>
      <source>Subelement highlight</source>
      <translation>Ressaltat de subelement</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="64"/>
      <source>Highlight the subelements of the selected objects, so that they can then be edited with the move, rotate, and scale tools.</source>
      <translation>Ressalta els subelements dels objectes seleccionats, i axió puguin ser editats amb les eines moure, rotar i escalar.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Text</name>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="60"/>
      <source>Text</source>
      <translation>Text</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="63"/>
      <source>Creates a multi-line annotation. CTRL to snap.</source>
      <translation>Crea una anotació multilínia. CTRL per ajustar.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleConstructionMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="99"/>
      <source>Toggle construction mode</source>
      <translation>Activar/desactivar el mode de construcció</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="102"/>
      <source>Toggles the Construction mode.
When this is active, the following objects created will be included in the construction group, and will be drawn with the specified color and properties.</source>
      <translation>Commuta el mode de Construcció.
Quan s'activa, els objectes creats a continuació seran inclosos en el grup de construcció, i seran dibuixats amb el color i les propietats especificades.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleContinueMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="131"/>
      <source>Toggle continue mode</source>
      <translation>Commuta el mode continuar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="133"/>
      <source>Toggles the Continue mode.
When this is active, any drawing tool that is terminated will automatically start again.
This can be used to draw several objects one after the other in succession.</source>
      <translation>Commuta el mode Continuar.
Quan està actiu, qualsevol eina de dibuix es torna a iniciar automàticament quan acaba.
Això es pot emprar per a dibuixar diversos objectes un rere l'altre en successió.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleDisplayMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="166"/>
      <source>Toggle normal/wireframe display</source>
      <translation>Commuta la vista normal/filferro</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="168"/>
      <source>Switches the display mode of selected objects from flatlines to wireframe and back.
This is helpful to quickly visualize objects that are hidden by other objects.
This is intended to be used with closed shapes and solids, and doesn't affect open wires.</source>
      <translation>Canvia el mode de visualització dels objectes seleccionats de línies planes a model de filferro i viceversa.
És útil per visualitzar ràpidament objectes ocults rere d'altres objectes.
Està pensat per a usar-se amb formes tancades i sòlids, i no afecta a filferros oberts.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleGrid</name>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="58"/>
      <source>Toggle grid</source>
      <translation>Commuta la graella</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="59"/>
      <source>Toggles the Draft grid on and off.</source>
      <translation>Commuta la graella d'Esbós.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Trimex</name>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="76"/>
      <source>Trimex</source>
      <translation>Trimex</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="82"/>
      <source>Trims or extends the selected object, or extrudes single faces.
CTRL snaps, SHIFT constrains to current segment or to normal, ALT inverts.</source>
      <translation>Retalla o allarga l'objecte seleccionat, o extrueix cares individuals.
CTRL per ajustar, MAJÚS per restringir al segment actual o a la normal, ALT per invertir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Upgrade</name>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="58"/>
      <source>Upgrade</source>
      <translation>Promociona</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="61"/>
      <source>Upgrades the selected objects into more complex shapes.
The result of the operation depends on the types of objects, which may be able to be upgraded several times in a row.
For example, it can join the selected objects into one, convert simple edges into parametric polylines,
convert closed edges into filled faces and parametric polygons, and merge faces into a single face.</source>
      <translation>Promociona els objectes seleccionats a formes més complexes.
El. resultat de l'operació depèn del tipus dels objectes, que poden ser promocionats diverses vegades seguides.
Per exemple, pot unir objectes seleccionats en un de sol, convertir simples arestes en polilínies paramètriques, convertir arestes tancades en cares amb ompliment i polígons paramètrics, i combinar cares en una sola cara.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Wire</name>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="306"/>
      <source>Polyline</source>
      <translation>Polilínia</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="309"/>
      <source>Creates a multiple-points line (polyline). CTRL to snap, SHIFT to constrain.</source>
      <translation>Crea una línia de múltiples punts (polilínia). CTRL per ajustar, MAJÚS per restringir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_WireToBSpline</name>
    <message>
      <location filename="../../draftguitools/gui_wire2spline.py" line="64"/>
      <source>Wire to B-spline</source>
      <translation>Filferro a B-spline</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_wire2spline.py" line="67"/>
      <source>Converts a selected polyline to a B-spline, or a B-spline to a polyline.</source>
      <translation>Converteix una polilínia seleccionada a B-spline, o una B-spline a polilínia.</translation>
    </message>
  </context>
  <context>
    <name>Draft_WorkingPlaneProxy</name>
    <message>
      <location filename="../../draftguitools/gui_planeproxy.py" line="50"/>
      <source>Create working plane proxy</source>
      <translation>Crea un proxy de pla de treball</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_planeproxy.py" line="52"/>
      <source>Creates a proxy object from the current working plane.
Once the object is created double click it in the tree view to restore the camera position and objects' visibilities.
Then you can use it to save a different camera position and objects' states any time you need.</source>
      <translation>Crea un objecte proxy del pla de treball actual.
Un cop l'objecte és creat, feu doble clic a la vista d'arbre per a restaurar la posició de càmera les visibilitats de l'objecte.
Aleshores, podeu usar-lo per desar posicions de càmera i estats d'objectes sempre que sigui necessari.</translation>
    </message>
  </context>
  <context>
    <name>Form</name>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="14"/>
      <source>Working plane setup</source>
      <translation>Configuració del pla de treball</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="20"/>
      <source>Select a face or working plane proxy or 3 vertices.
Or choose one of the options below</source>
      <translation>Seleccioneu una cara o un pla de treball intermediari o 3 vèrtexs. O trieu una de les opcions següents</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="31"/>
      <source>Sets the working plane to the XY plane (ground plane)</source>
      <translation>Estableix el pla de treball al pla XY (pla del terra)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="34"/>
      <source>Top (XY)</source>
      <translation>Part superior (XY)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="41"/>
      <source>Sets the working plane to the XZ plane (front plane)</source>
      <translation>Estableix el pla de treball al pla XZ (pla frontal)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="44"/>
      <source>Front (XZ)</source>
      <translation>Part de davant (XZ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="51"/>
      <source>Sets the working plane to the YZ plane (side plane)</source>
      <translation>Estableix el pla de treball al pla YZ (pla lateral)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="54"/>
      <source>Side (YZ)</source>
      <translation>Al costat (YZ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="61"/>
      <source>Sets the working plane facing the current view</source>
      <translation>Estableix el pla de treball encarat amb la vista actual</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="64"/>
      <source>Align to view</source>
      <translation>Alinea a la vista</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="71"/>
      <source>The working plane will align to the current
view each time a command is started</source>
      <translation>El pla de treball s’alinearà a la vista actual cada vegada que s'inicia una ordre</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="78"/>
      <source>Automatic</source>
      <translation>Automàtica</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="87"/>
      <source>Offset</source>
      <translation>Equidistancia (ofset)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="94"/>
      <source>An optional offset to give to the working plane
above its base position. Use this together with one
of the buttons above</source>
      <translation>Una separació opcional per al pla de treball damunt de la seva posició de base. Utilitzeu-ho juntament amb un dels botons de dalt</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="106"/>
      <location filename="../ui/TaskSelectPlane.ui" line="118"/>
      <source>If this is selected, the working plane will be
centered on the current view when pressing one
of the buttons above</source>
      <translation>Si aquesta opció està seleccionada, el pla de treball se centrarà en la vista actual en prémer un dels botons de dalt</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="111"/>
      <source>Center plane on view</source>
      <translation>Pla central de la vista</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="135"/>
      <source>Or select a single vertex to move the current
working plane without changing its orientation.
Then, press the button below</source>
      <translation>O seleccioneu un vèrtex simple per a moure el pla de treball actual sense canviar la seua orientació. A continuació, premeu el botó següent</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="147"/>
      <source>Moves the working plane without changing its
orientation. If no point is selected, the plane
will be moved to the center of the view</source>
      <translation>Mou el pla de treball sense canviar la seva orientació. Si no hi ha cap punt seleccionat, el pla es mourà al centre de la vista</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="152"/>
      <source>Move working plane</source>
      <translation>Mou el pla de treball</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="161"/>
      <location filename="../ui/TaskSelectPlane.ui" line="171"/>
      <source>The spacing between the smaller grid lines</source>
      <translation>L'espaiat entre les línies de la graella més xicotetes</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="164"/>
      <source>Grid spacing</source>
      <translation>Espaiat de la quadrícula</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="181"/>
      <location filename="../ui/TaskSelectPlane.ui" line="191"/>
      <source>The number of squares between each main line of the grid</source>
      <translation>El nombre de quadrats entre cada línia principal de la graella</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="184"/>
      <source>Main line every</source>
      <translation>Línia principal cada</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="198"/>
      <source>Grid extension</source>
      <translation>Extensió de la graella</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="205"/>
      <source> lines</source>
      <translation> línies</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="218"/>
      <location filename="../ui/TaskSelectPlane.ui" line="230"/>
      <source>The distance at which a point can be snapped to
when approaching the mouse. You can also change this
value by using the [ and ] keys while drawing</source>
      <translation>La distància a la qual es pot ajustar un punt
en acostar-se al ratolí. També podeu canviar aquest
valor mitjançant les tecles [ i ] mentre es dibuixa</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="223"/>
      <source>Snapping radius</source>
      <translation>Ajust Radi</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="241"/>
      <source>Centers the view on the current working plane</source>
      <translation>Centra la vista en el pla de treball actual</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="244"/>
      <source>Center view</source>
      <translation>Centra la vista</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="251"/>
      <source>Resets the working plane to its previous position</source>
      <translation>Reinicia el pla de treball a la seva posició anterior</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="254"/>
      <source>Previous</source>
      <translation>Anterior</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="14"/>
      <source>Style settings</source>
      <translation>Ajustaments d'estil</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="22"/>
      <source>Fills the values below with a stored style preset</source>
      <translation type="unfinished">Fills the values below with a stored style preset</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="26"/>
      <source>Load preset</source>
      <translation type="unfinished">Load preset</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="40"/>
      <source>Save current style as a preset...</source>
      <translation type="unfinished">Save current style as a preset...</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="57"/>
      <source>Lines and faces</source>
      <translation>Línies i cares</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="66"/>
      <source>Line color</source>
      <translation>Color de línia</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="73"/>
      <source>The color of lines</source>
      <translation>El color de les línies</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="80"/>
      <source>Line width</source>
      <translation>Amplada de línia</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="87"/>
      <source> px</source>
      <translation> px</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="94"/>
      <source>Draw style</source>
      <translation>Estil de dibuix</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="101"/>
      <source>The line style</source>
      <translation>L'estil de la línia</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="105"/>
      <source>Solid</source>
      <translation>Sòlid</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="110"/>
      <source>Dashed</source>
      <translation>Discontínua</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="115"/>
      <source>Dotted</source>
      <translation>Puntejada</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="120"/>
      <source>DashDot</source>
      <translation>Guió punt</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="128"/>
      <source>Display mode</source>
      <translation>Mode de visualització</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="135"/>
      <source>The display mode for faces</source>
      <translation>El tipus de visualització per les cares</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="139"/>
      <source>Flat Lines</source>
      <translation>Linies planes</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="144"/>
      <source>Wireframe</source>
      <translation>Model de filferro</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="149"/>
      <source>Shaded</source>
      <translation>Ombrejat</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="154"/>
      <source>Points</source>
      <translation>Punts</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="162"/>
      <source>Shape color</source>
      <translation>Color de la forma</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="169"/>
      <source>The color of faces</source>
      <translation>El color de les cares</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="176"/>
      <source>Transparency</source>
      <translation>Transparència</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="183"/>
      <source>The transparency of faces</source>
      <translation>La transparència de les cares</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="186"/>
      <source> %</source>
      <translation> %</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="199"/>
      <source>Annotations</source>
      <translation>Anotacions</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="205"/>
      <source>Text font</source>
      <translation>Tius de lletra</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="218"/>
      <source>The font to use for texts and dimensions</source>
      <translation>Font a emprar per a textos i dimensions</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="225"/>
      <source>Text size</source>
      <translation>Mida del text</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="232"/>
      <source>The size of texts and dimension texts</source>
      <translation>Mida dels textos i del textos de les cotes</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="242"/>
      <source>Text spacing</source>
      <translation>Espaiat de text</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="249"/>
      <source>The space between the text and the dimension line</source>
      <translation type="unfinished">The space between the text and the dimension line</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="259"/>
      <source>Text color</source>
      <translation>Color del text</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="266"/>
      <source>The color of texts and dimension texts</source>
      <translation>El color dels textos i dels textos de les cotes</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="273"/>
      <source>Line spacing</source>
      <translation>Interlineat</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="280"/>
      <source>The spacing between different lines of text</source>
      <translation type="unfinished">The spacing between different lines of text</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="287"/>
      <source>Arrow style</source>
      <translation>Estil de la fletxa</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="294"/>
      <source>The type of dimension arrows</source>
      <translation>El tipus de les fletxes de les cotes</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="298"/>
      <source>Dot</source>
      <translation>Punt</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="303"/>
      <source>Circle</source>
      <translation>Cercle</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="308"/>
      <source>Arrow</source>
      <translation>Fletxa</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="313"/>
      <source>Tick</source>
      <translation>Tick</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="318"/>
      <source>Tick-2</source>
      <translation>Tick-2</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="326"/>
      <source>Arrow size</source>
      <translation>Mida de la fletxa</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="333"/>
      <source>The size of dimension arrows</source>
      <translation>Mida de les fletxes de les cotes</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="343"/>
      <source>Show unit</source>
      <translation>Mostra les unitats</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="350"/>
      <source>If the unit suffix is shown on dimension texts or not</source>
      <translation>Si es mostra o no el sufix d'unitat als textos de les cotes</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="363"/>
      <source>Unit override</source>
      <translation>Substitució d'unitat</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="370"/>
      <source>The unit to use for dimensions. Leave blank to use current FreeCAD unit</source>
      <translation>Unitat que s'utilitzarà per les cotes. Deixeu-la en blanc per usar l'unitat actual del FreeCAD</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="382"/>
      <source>Apply above style to selected object(s)</source>
      <translation type="unfinished">Apply above style to selected object(s)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="385"/>
      <source>Selected</source>
      <translation>Seleccionat</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="397"/>
      <source>Texts/dims</source>
      <translation type="unfinished">Texts/dims</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="14"/>
      <source>Form</source>
      <translation>Forma</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="20"/>
      <source>PAT file:</source>
      <translation type="unfinished">PAT file:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="27"/>
      <source>pattern files (*.pat)</source>
      <translation type="unfinished">pattern files (*.pat)</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="34"/>
      <source>Pattern:</source>
      <translation type="unfinished">Pattern:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="44"/>
      <source>Scale</source>
      <translation>Escala</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="64"/>
      <source>Rotation:</source>
      <translation>Rotació:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="71"/>
      <source>°</source>
      <translation>º</translation>
    </message>
  </context>
  <context>
    <name>Gui::Dialog::DlgSettingsDraft</name>
    <message>
      <location filename="../ui/preferences-draft.ui" line="14"/>
      <source>General settings</source>
      <translation>Configuració general</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="32"/>
      <source>General Draft Settings</source>
      <translation>Configuracions Generals de Projecte</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="40"/>
      <source>Default working plane</source>
      <translation>Pla de treball per defecte</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="67"/>
      <source>None</source>
      <translation>Cap</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="72"/>
      <source>XY (Top)</source>
      <translation>XY (dalt)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="77"/>
      <source>XZ (Front)</source>
      <translation>XZ (Front.)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="82"/>
      <source>YZ (Side)</source>
      <translation>YZ (costat)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="94"/>
      <source>Internal precision level</source>
      <translation>Nivell de precisió interna</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="120"/>
      <source>The number of decimals in internal coordinates operations (for ex. 3 = 0.001). Values between 6 and 8 are usually considered the best trade-off among FreeCAD users.</source>
      <translation>El nombre de decimals en les operacions de coordenades internes (p. ex. 3 = 0,001). Els usuaris de FreeCAD consideren el millor rang els valors entre 6 i 8.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="143"/>
      <source>Tolerance</source>
      <translation>Tolerància</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="169"/>
      <source>This is the value used by functions that use a tolerance.
Values with differences below this value will be treated as same. This value will be obsoleted soon so the precision level above controls both.</source>
      <translation>Aquest és el valor utilitzat per a funcions que utilitzen una tolerància.
Els valors amb diferències per sota d'aquest valor es tractaran com a iguals. Aquest valor aviat serà obsolet, així que el nivell de precisió superior controla tots dós.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="194"/>
      <source>If this option is checked, the layers drop-down list will also show groups, allowing you to automatically add objects to groups too.</source>
      <translation>Si es marca aquesta opció, la llista desplegable de capes també mostrarà grups, cosa que us permetrà afegir objectes automàticament a grups.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="197"/>
      <source>Show groups in layers list drop-down button</source>
      <translation>Mostra grups en el botó de la llista desplegable de capes</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="216"/>
      <source>Draft tools options</source>
      <translation>Opcions d'eines d'Esbós</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="230"/>
      <source>When drawing lines, set focus on Length instead of X coordinate.
This allows to point the direction and type the distance.</source>
      <translation>En dibuixar línies, fixa el focus en la llargària en lloc de en les coordenades X.
Això permet assenyalar la direcció i introduir la distància.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="234"/>
      <source>Set focus on Length instead of X coordinate</source>
      <translation>Estableix el focus en la llargària en lloc de en les coordenades X</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="247"/>
      <source>Normally, after copying objects, the copies get selected.
If this option is checked, the base objects will be selected instead.</source>
      <translation>Normalment, després de copiar objectes, selecciona les còpies. Si aquesta opció està marcada, se seleccionaran, en el seu lloc, els objectes base.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="251"/>
      <source>Select base objects after copying</source>
      <translation>Permet seleccionar objectes base després de copiar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="264"/>
      <source>If this option is set, when creating Draft objects on top of an existing face of another object, the "Support" property of the Draft object will be set to the base object. This was the standard behaviour before FreeCAD 0.19</source>
      <translation>Si s'ha establert aquesta opció, en crear objectes Esbós a la part superior d'una cara existent d'altre objecte, la propietat «Suport» de l'objecte Esbós s'establirà en l'objecte base. Aquest era el comportament estàndard abans de FreeCAD 0.19</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="267"/>
      <source>Set the Support property when possible</source>
      <translation>Estableix la propietat Suport quan siga possible</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="280"/>
      <source>If this is checked, objects will appear as filled by default.
Otherwise, they will appear as wireframe</source>
      <translation>Si està marcada, els objectes apareixeran plens per defecte. En cas contrari, apareixeran com a model de filferro</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="284"/>
      <source>Fill objects with faces whenever possible</source>
      <translation>Omplir els objectes amb cares sempre que sigui possible</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="300"/>
      <source>If this is checked, copy mode will be kept across command,
otherwise commands will always start in no-copy mode</source>
      <translation>Si està marcada, el mode copia es mantindrà d'una ordre a l'altra, en cas contrari les ordres sempre s'iniciaran en el mode no-còpia</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="304"/>
      <source>Global copy mode</source>
      <translation>Mode de còpia global</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="320"/>
      <source>Force Draft Tools to create Part primitives instead of Draft objects.
Note that this is not fully supported, and many object will be not editable with Draft Modifiers.</source>
      <translation>Força les eines d'esbós a crear primitives de peça enlloc d'objectes esbós.
Tingueu en compte que això no és totalment compatible i que molts objectes no es poden editar amb els modificadors d'esbós.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="324"/>
      <source>Use Part Primitives when available</source>
      <translation>Utilitza Primitives de peça quan sigui possible</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="341"/>
      <source>Prefix labels of Clones with:</source>
      <translation>Etiquetes dels clons del prefix:</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="372"/>
      <source>Construction Geometry</source>
      <translation>Geometria de construcció</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="380"/>
      <source>Construction group name</source>
      <translation>Nom del grup de construcció</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="387"/>
      <source>This is the default group name for construction geometry</source>
      <translation>Aquest es el nom del grup predeterminat per la geometria de la construcció</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="390"/>
      <source>Construction</source>
      <translation>Construcció</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="407"/>
      <source>Construction geometry color</source>
      <translation>Color de geometria de construcció</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="427"/>
      <source>This is the default color for objects being drawn while in construction mode.</source>
      <translation>Aquest es el color predeterminat per objectes que estan sent dibuixats en el mode de construcció.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="14"/>
      <source>Visual settings</source>
      <translation>Escenes visuals</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="26"/>
      <source>Visual Settings</source>
      <translation>Escenes visuals</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="34"/>
      <source>Snap symbols style</source>
      <translation>Estil de símbols de ajust</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="54"/>
      <source>Draft classic style</source>
      <translation>Estil clàssic</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="59"/>
      <source>Bitsnpieces style</source>
      <translation>Estil Bitsnpieces</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="80"/>
      <source>Color</source>
      <translation>Color</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="87"/>
      <source>The default color for snap symbols</source>
      <translation>El color predeterminat per símbols de ajust ràpid</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="111"/>
      <source>Check this if you want to use the color/linewidth from the toolbar as default</source>
      <translation>Triar aquesta opció si desitja utilitzar el color/ample de línia de la barra d'eines per defecte</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="114"/>
      <source>Save current color and linewidth across sessions</source>
      <translation>Guardar color actual i linewidth a través de les sessions</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="131"/>
      <source>If checked, a widget indicating the current working plane orientation appears during drawing operations</source>
      <translation>Si està marcada, apareix un giny que indica l'orientació de pla de treball actual durant les operacions de dibuix</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="134"/>
      <source>Show Working Plane tracker</source>
      <translation>Mostra el pla de treball rastrejador</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="151"/>
      <source>Default template sheet</source>
      <translation>Full de plantilla per defecte</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="177"/>
      <source>The default template to use when creating a new drawing sheet</source>
      <translation>La plantilla per defecte a utilitzar quan es crea un nou full de dibuix</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="194"/>
      <source>Alternate SVG patterns location</source>
      <translation type="unfinished">Alternate SVG patterns location</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="220"/>
      <source>Here you can specify a directory with custom SVG files containing &lt;pattern&gt; definitions to be added to the standard patterns</source>
      <translation type="unfinished">Here you can specify a directory with custom SVG files containing &lt;pattern&gt; definitions to be added to the standard patterns</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="237"/>
      <source>SVG pattern resolution</source>
      <translation type="unfinished">SVG pattern resolution</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="257"/>
      <source>The resolution to draw the patterns in. Default value is 128. Higher values give better resolutions, lower values make drawing faster</source>
      <translation>Resolució a la qual es dibuixaran els patrons. El valor per defecte és 128. Valors més alts donen resolucions millors, valors més baixos fan el dibuix més ràpid</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="280"/>
      <source>SVG pattern default size</source>
      <translation type="unfinished">SVG pattern default size</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="300"/>
      <source>The default size for SVG patterns</source>
      <translation type="unfinished">The default size for SVG patterns</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="326"/>
      <source>Check this if you want to preserve colors of faces while doing downgrade and upgrade (splitFaces and makeShell only)</source>
      <translation>Marcar si voleu preservar el color de les cares mentre s'estigui revertint o actualitzant (sols splitFaces i makeShell)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="329"/>
      <source>Preserve colors of faces during downgrade/upgrade</source>
      <translation>Preserva el color de les cares durant la downgrade/upgrade</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="346"/>
      <source>Check this if you want the face names to derive from the originating object name and vice versa while doing downgrade/upgrade (splitFaces and makeShell only)</source>
      <translation>Marqueu això si voleu que els noms de les cares derivin del nom de l'objecte d'origen i viceversa quan s'estigui revertint o actualitzant (sols splitFaces i makeShell)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="349"/>
      <source>Preserve names of faces during downgrade/upgrade</source>
      <translation>Preserva el nom de les cares durant la reversió i l'actualització</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="367"/>
      <source>Drawing view line definitions</source>
      <translation>Definicions de línia de vista de dibuix</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="375"/>
      <source>Dashed line definition</source>
      <translation>Definició de la línia discontínua</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="395"/>
      <location filename="../ui/preferences-draftvisual.ui" line="438"/>
      <location filename="../ui/preferences-draftvisual.ui" line="481"/>
      <source>An SVG linestyle definition</source>
      <translation>Una definició d'estil de línia SVG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="398"/>
      <source>0.09,0.05</source>
      <translation>0.09,0.05</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="418"/>
      <source>Dashdot line definition</source>
      <translation>Dashdot definició de línia</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="441"/>
      <source>0.09,0.05,0.02,0.05</source>
      <translation>0.09,0.05,0.02,0.05</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="461"/>
      <source>Dotted line definition</source>
      <translation>Definició de la línia de punts</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="484"/>
      <source>0.02,0.02</source>
      <translation>0.02,0.02</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="14"/>
      <source>Texts and dimensions</source>
      <translation>Textos i dimensions</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="26"/>
      <source>Text settings</source>
      <translation>Configuració de text</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="34"/>
      <source>Font family</source>
      <translation>Tipus de Font</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="47"/>
      <source>This is the default font name for all Draft texts and dimensions.
It can be a font name such as "Arial", a default style such as "sans", "serif"
or "mono", or a family such as "Arial,Helvetica,sans" or a name with a style
such as "Arial:Bold"</source>
      <translation>Aquest és el nom de tipus de lletra per defecte per a tots els textos d'Esbós i les cotes. Pot ser un nom de lletra com "Arial", un estil per defecte com "sans", "serif" o "mono" o una família com "Arial, Helvètica", una tipografia sans o un nom amb un estil com «Arial: negreta"</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="59"/>
      <source>Internal font</source>
      <translation>Tipus de lletra interna</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="76"/>
      <source>Font size</source>
      <translation>Mida del tipus de lletra</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="89"/>
      <source>Default height for texts and dimensions</source>
      <translation>Alçada d'omissió per a textos i dimensions</translation>
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
      <translation>Configuració de la dimensió</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="124"/>
      <source>Display mode</source>
      <translation>Mode de visualització</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="144"/>
      <source>text above (2D)</source>
      <translation>text (2D) per sobre</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="149"/>
      <source> text inside (3D)</source>
      <translation> text dins (3D)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="161"/>
      <source>Number of decimals</source>
      <translation>Nombre de decimals</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="201"/>
      <source>Extension lines size</source>
      <translation>Tamany de les linies d’extensiò</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="208"/>
      <source>The default size of dimensions extension lines</source>
      <translation>La mida per defecte de línies d'extensió de dimensions</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="237"/>
      <source>Extension line overshoot</source>
      <translation>Superació de la línia d'extensió</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="244"/>
      <source>The default length of extension line above dimension line</source>
      <translation>La llargària per defecte de l'extensió de la línia per sobre de la línia de dimensió</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="273"/>
      <source>Dimension line overshoot</source>
      <translation>Superació de la línia de dimensió</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="280"/>
      <source>The default distance the dimension line is extended past extension lines</source>
      <translation>La distància per defecte de la línia de dimensió s'estén més enllà de les línies d'extensió</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="309"/>
      <source>Arrows style</source>
      <translation>Estil de fletxes</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="323"/>
      <source>Dot</source>
      <translation>Punt</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="328"/>
      <source>Circle</source>
      <translation>Cercle</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="333"/>
      <source>Arrow</source>
      <translation>Fletxa</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="338"/>
      <source>Tick</source>
      <translation>Tick</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="343"/>
      <source>Tick-2</source>
      <translation>Tick-2</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="355"/>
      <source>Arrows size</source>
      <translation>Mida de les fletxes</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="362"/>
      <source>The default size of arrows</source>
      <translation>La mida per defecte de fletxes</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="388"/>
      <source>Text orientation</source>
      <translation>Orientació de text</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="395"/>
      <source>This is the orientation of the dimension texts when those dimensions are vertical. Default is left, which is the ISO standard.</source>
      <translation>Això és l'orientació dels textos dimensió quan aquestes dimensions són verticals. Es deixa per defecte, que és l'estàndard ISO.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="405"/>
      <source>Left (ISO standard)</source>
      <translation>Esquerra (norma ISO)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="410"/>
      <source>Right</source>
      <translation>Dreta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="422"/>
      <source>Text spacing</source>
      <translation>Espaiat de text</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="429"/>
      <source>The space between the dimension line and the dimension text</source>
      <translation>L'espai entre la línia de la dimensió i el text de la dimensió</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="455"/>
      <source>Show the unit suffix in dimensions</source>
      <translation>Mostrar el sufix d'unitat en dimensions</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="475"/>
      <source>Override unit</source>
      <translation>Sobreescriu la unitat</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="495"/>
      <source>By leaving this field blank, the dimension measurements will be shown in the current unit defined in FreeCAD. By indicating a unit here such as m or cm, you can force new dimensions to be shown in that unit.</source>
      <translation>Si el camp es deixa en blanc, les mesures de les cotes es mostraran en l'unitat actual definida a FreeCAD. Si s'indica una unitat aqui, com ara m o cm, podeu forçar a que es mostrin les cotes en aquesta unitat.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="513"/>
      <source>ShapeString settings</source>
      <translation>Configuració ShapeString</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="521"/>
      <source>Default ShapeString font file</source>
      <translation>Fitxer de tipus de lletra per defecte ShapeString</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="534"/>
      <source>Select a font file</source>
      <translation>Seleccioneu un fitxer de tipus de lletra</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="14"/>
      <source>SVG</source>
      <translation>SVG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="40"/>
      <source>Import style</source>
      <translation>Estil d'importació</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="47"/>
      <source>Method chosen for importing SVG object color to FreeCAD</source>
      <translation>Aquest és el mètode escollit per a importar el color d'objecte SVG a FreeCAD</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="60"/>
      <source>None (fastest)</source>
      <translation>Cap (més ràpid)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="65"/>
      <source>Use default color and linewidth</source>
      <translation>Ús omissió color i linewidth</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="70"/>
      <source>Original color and linewidth</source>
      <translation>Color original i linewidth</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="82"/>
      <source>If checked, no units conversion will occur.
One unit in the SVG file will translate as one millimeter. </source>
      <translation>Si està marcada, no es produeix cap conversió d'unitats. Una unitat en el fitxer SVG es traduirà com un mil·límetre. </translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="86"/>
      <source>Disable units scaling</source>
      <translation>Deshabilitar el escalat de unitats</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="121"/>
      <source>Export style</source>
      <translation>Estil d'exportació</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="128"/>
      <source>Style of SVG file to write when exporting a sketch</source>
      <translation>Estil que s'usarà per escriure el fitxer SVG quan s'exporti un croquis</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="141"/>
      <source>Translated (for print &amp; display)</source>
      <translation>Traduit (per impressió i pantalla)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="146"/>
      <source>Raw (for CAM)</source>
      <translation>Raw (for CAM)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="158"/>
      <source>All white lines will appear in black in the SVG for better readability against white backgrounds</source>
      <translation>Totes les línies blanques apareixen en negre en l'SVG per a una millor llegibilitat amb un fons blancs</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="161"/>
      <source>Translate white line color to black</source>
      <translation>Traduir el color de línia de blanc a negre</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="181"/>
      <source>Max segment length for discretized arcs</source>
      <translation>Longitud de segment de Max per arcs discretized</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="204"/>
      <source>Versions of Open CASCADE older than version 6.8 don't support arc projection.
In this case arcs will be discretized into small line segments.
This value is the maximum segment length. </source>
      <translation>Les versions d'Open CASCADE anteriors a la versió 6.8 no admeten la projecció d'arc. En aquest cas, els arcs es poden discretitzar en xicotets segments de línia. Aquest valor és la llargària màxima del segment. </translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="14"/>
      <source>OCA</source>
      <translation>OCA</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="46"/>
      <source>Check this if you want the areas (3D faces) to be imported too.</source>
      <translation>Seleccionar aquesta opció si vol que les áreas (cares 3D) també s' importin.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="49"/>
      <source>Import OCA areas</source>
      <translation>Importació OCA àrees</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="14"/>
      <source>DXF</source>
      <translation>DXF</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="35"/>
      <source>This preferences dialog will be shown when importing/ exporting DXF files</source>
      <translation>Aquest diàleg de preferències es mostrarà quan importeu/exporteu fitxers DXF</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="38"/>
      <source>Show this dialog when importing and exporting</source>
      <translation>Mostra aquest diàleg durant la importació i l'exportació</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="51"/>
      <source>Python importer is used, otherwise the newer C++ is used.
Note: C++ importer is faster, but is not as featureful yet</source>
      <translation>S'utilitza importador de Python, en cas contrari s'utilitza el C ++ més recent.
Nota: l’importador C ++ és més ràpid, però encara no té tantes funcionalitats</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="55"/>
      <source>Use legacy python importer</source>
      <translation>Importador de Python llegat d'ús</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="71"/>
      <source>Python exporter is used, otherwise the newer C++ is used.
Note: C++ exporter is faster, but is not as featureful yet</source>
      <translation type="unfinished">Python exporter is used, otherwise the newer C++ is used.
Note: C++ exporter is faster, but is not as featureful yet</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="75"/>
      <source>Use legacy python exporter</source>
      <translation>Utilitza l'exportador python llegat</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="88"/>
      <source>Automatic update (legacy importer only)</source>
      <translation>Actualització automàtica (llegat importador només)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="96"/>
      <source>Allow FreeCAD to download the Python converter for DXF import and export.
You can also do this manually by installing the "dxf_library" workbench
from the Addon Manager.</source>
      <translation>Permet a FreeCAD descarregar el convertidor Python per a la importació i exportació de DXF.
També podeu fer-ho manualment instal·lant el banc de treball «dxf_library»
del gestor de complements.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="101"/>
      <source>Allow FreeCAD to automatically download and update the DXF libraries</source>
      <translation>Permeten a FreeCAD automàticament descarregar i actualitzar les biblioteques DXF</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="26"/>
      <location filename="../ui/preferences-dxf.ui" line="119"/>
      <location filename="../ui/preferences-svg.ui" line="26"/>
      <source>Import options</source>
      <translation>Opcions d'importació</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="140"/>
      <source>Note: Not all the options below are used by the new importer yet</source>
      <translation>Nota: No totes les opcions següents són utilitzats per l'importador nou encara</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="149"/>
      <source>Import</source>
      <translation>Importa</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="156"/>
      <source>If unchecked, texts and mtexts won't be imported</source>
      <translation>Si està desactivada, no importa textos ni mtextos</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="159"/>
      <source>texts and dimensions</source>
      <translation>textos i dimensions</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="172"/>
      <source>If unchecked, points won't be imported</source>
      <translation>Si està desactivada, no s'importen els punts</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="175"/>
      <source>points</source>
      <translation>punts</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="188"/>
      <source>If checked, paper space objects will be imported too</source>
      <translation>Si està activada, els objectes de l'espai paper també s'importaran</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="191"/>
      <source>layouts</source>
      <translation>disposicions</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="204"/>
      <source>If you want the non-named blocks (beginning with a *) to be imported too</source>
      <translation>Si voleu que els blocs sense nom (que comencen amb un *) també s'importen</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="207"/>
      <source>*blocks</source>
      <translation>* blocs</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="224"/>
      <source>Create</source>
      <translation>Crea</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="231"/>
      <source>Only standard Part objects will be created (fastest)</source>
      <translation>Només es crearan objectes peça estàndard (més ràpid)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="234"/>
      <source>simple Part shapes</source>
      <translation>formes simples de Peça</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="250"/>
      <source>Parametric Draft objects will be created whenever possible</source>
      <translation>Els objectes Esbós paramètrics es crearan quan sigui possible</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="253"/>
      <source>Draft objects</source>
      <translation>Projecte objectes</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="266"/>
      <source>Sketches will be created whenever possible</source>
      <translation>Es crearan croquis quan sigui possible</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="269"/>
      <source>Sketches</source>
      <translation>Croquis</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="289"/>
      <source>Scale factor to apply to imported files</source>
      <translation>Factor d'escala per aplicar a fitxers importats</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="309"/>
      <source>Scale factor to apply to DXF files on import.
The factor is the conversion between the unit of your DXF file and millimeters.
Example: for files in millimeters: 1, in centimeters: 10,
                             in meters: 1000, in inches: 25.4, in feet: 304.8</source>
      <translation>Factor d'escala a aplicar als arxius DXF durant la importació. El factor és la conversió entre la unitat del seu fitxer DXF i mil·límetres. Ex: per a fitxers en mil·límetres: 1, en centímetres: 10, en metres: 1000, en polzades: 25.4, en peus: 304.8</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="338"/>
      <source>Colors will be retrieved from the DXF objects whenever possible.
Otherwise default colors will be applied. </source>
      <translation>Es recuperen els colors dels objectes DXF sempre que siga possible. En cas contrari s'aplica el colors per defecte. </translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="342"/>
      <source>Get original colors from the DXF file</source>
      <translation>Obtenir colors originals de l'arxiu DXF</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="359"/>
      <source>FreeCAD will try to join coincident objects into wires.
Note that this can take a while!</source>
      <translation>FreeCAD intentarà unir objectes coincidents en filferros. Pot ser un procés lent!</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="363"/>
      <source>Join geometry</source>
      <translation>Uneix-te a geometria</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="380"/>
      <source>Objects from the same layers will be joined into Draft Blocks,
turning the display faster, but making them less easily editable </source>
      <translation>Els objectes de les mateixes capes s'uniran en Blocs d'Esbós, la visualització serà més ràpida, però l'edició serà més difícil </translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="384"/>
      <source>Group layers into blocks</source>
      <translation>Grup de capes en blocs</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="401"/>
      <source>Imported texts will get the standard Draft Text size,
instead of the size they have in the DXF document</source>
      <translation>Els textos importats tindran la mida estàndard d'Esborrany, en lloc de la mida que tenen en el document DXF</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="405"/>
      <source>Use standard font size for texts</source>
      <translation>Utilitza la mida de font estàndard per a textos</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="422"/>
      <source>If this is checked, DXF layers will be imported as Draft Layers</source>
      <translation>Si està marcada, s'importaran capes DXF com a capes d'Esbós</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="425"/>
      <source>Use Layers</source>
      <translation>Utilitza les capes</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="445"/>
      <source>Hatches will be converted into simple wires</source>
      <translation>Les trames es convertiran en filferros simples</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="448"/>
      <source>Import hatch boundaries as wires</source>
      <translation>Importar ombrejats frontera com filferros</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="465"/>
      <source>If polylines have a width defined, they will be rendered
as closed wires with correct width</source>
      <translation>Si les polilínies tenen una amplària definida, es renderitzaran com a filferros tancats amb l'amplària correcta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="469"/>
      <source>Render polylines with width</source>
      <translation>Representar Polilínies amb amplada</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="486"/>
      <source>Ellipse export is poorly supported. Use this to export them as polylines instead.</source>
      <translation>Exportació d'el·lipses poc compatible. En el seu lloc, utilitzeu això per a exportar-les com a polilínies.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="489"/>
      <source>Treat ellipses and splines as polylines</source>
      <translation>Tractar d'el·lipses i splines com Polilínies</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="518"/>
      <source>Max Spline Segment:</source>
      <translation>Segment de Spline Màxim:</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="528"/>
      <source>Maximum length of each of the polyline segments.
If it is set to '0' the whole spline is treated as a straight segment.</source>
      <translation>Longitud màxima de cadascun dels segments de la polilínia. Si s'estableix en «0», l'spline sencer es tracta com un segment recte.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="559"/>
      <location filename="../ui/preferences-svg.ui" line="107"/>
      <source>Export options</source>
      <translation>Opcions d'exportació</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="567"/>
      <source>All objects containing faces will be exported as 3D polyfaces</source>
      <translation>Tots els objectes que continguen cares s'exportaran com a multifacetes 3D</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="570"/>
      <source>Export 3D objects as polyface meshes</source>
      <translation>Exportar objectes 3D com polyface malles</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="587"/>
      <source>Drawing Views will be exported as blocks.
This might fail for post DXF R12 templates.</source>
      <translation>Les vistes de dibuix s’exportaran com a blocs. Això pot fallar per a les plantilles post DXF R12.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="591"/>
      <source>Export Drawing Views as blocks</source>
      <translation>Exportar el dibuix vistes com a blocs</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="611"/>
      <source>Exported objects will be projected to reflect the current view direction</source>
      <translation>Els objectes exportats es projectaran per a reflectir la direcció actual de la vista</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="614"/>
      <source>Project exported objects along current view direction</source>
      <translation>Projecte exportar objectes al llarg de la direcció de visualització</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="14"/>
      <source>Grid and snapping</source>
      <translation>Xarxa i l'ajust</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="35"/>
      <source>Snapping</source>
      <translation>Ajusta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="43"/>
      <source>If this is checked, snapping is activated without the need to press the snap mod key</source>
      <translation>Si està marcada, ajustament és activat sense la necessitat de prémer la tecla de snap mod</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="46"/>
      <source>Always snap (disable snap mod)</source>
      <translation>Sempre (Impossibiliti Snap mod)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="66"/>
      <source>Constrain mod</source>
      <translation>Mode de restricció</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="86"/>
      <source>The Constraining modifier key</source>
      <translation>La tecla modificadora de Restricció</translation>
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
      <translation>Snap mod</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="138"/>
      <source>The snap modifier key</source>
      <translation>La tecla modificador snap</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="173"/>
      <source>Alt mod</source>
      <translation>Alt mod</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="193"/>
      <source>The Alt modifier key</source>
      <translation>La tecla modificadora Alt</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="228"/>
      <source>If checked, the Snap toolbar will be shown whenever you use snapping</source>
      <translation>Si està marcada, es mostrarà la barra d'eines de Snap cada vegada que utilitzes ajustament</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="231"/>
      <source>Show Draft Snap toolbar</source>
      <translation>Mostra la varra d'eines d'Ajust de l'Esbós</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="251"/>
      <source>Hide Draft snap toolbar after use</source>
      <translation>Amaga la barra d'eines de projecte Snap després del seu ús</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="272"/>
      <source>Grid</source>
      <translation>Quadrícula</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="278"/>
      <source>If checked, a grid will appear when drawing</source>
      <translation>Si està marcada, una quadrícula apareixerà quan dibuix</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="281"/>
      <source>Use grid</source>
      <translation>Use grid</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="300"/>
      <source>If checked, the Draft grid will always be visible when the Draft workbench is active. Otherwise only when using a command</source>
      <translation>Si està marcada, la cuadricula d'esbós sempre serà visible quan Esbós estigui actiu. En cas contrari només quan s'usi una ordre</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="303"/>
      <source>Always show the grid</source>
      <translation>Sempre Mostra la quadrícula</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="319"/>
      <source>If checked, an additional border is displayed around the grid, showing the main square size in the bottom left border</source>
      <translation>Si es marca, es mostra una vora addicional al voltant de la graella, mostrant la mida del quadre principal a la vora inferior esquerra</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="322"/>
      <source>Show grid border</source>
      <translation>Mostra la vora de la graella</translation>
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
      <translation>Si es marca, la graella tindrà els seus dos eixos principals de color verd, vermell o blau quan coincideixin amb els eixos globals</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="360"/>
      <source>Use colored axes</source>
      <translation>Fes servir els eixos de colors</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="381"/>
      <source>Main lines every</source>
      <translation>Principals línies cada</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="404"/>
      <source>Mainlines will be drawn thicker. Specify here how many squares between mainlines.</source>
      <translation>Mainlines s'extrauran més gruixuda. Especifiqueu aquí quantes places entre mainlines.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="430"/>
      <source>Grid spacing</source>
      <translation>Espaiat de la quadrícula</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="453"/>
      <source>The spacing between each grid line</source>
      <translation>L'espaiat entre cada línia de la quadrícula</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="485"/>
      <source>Grid size</source>
      <translation>Mida de reixat</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="505"/>
      <source>The number of horizontal or vertical lines of the grid</source>
      <translation>El nombre de línies horitzontals o verticals de la quadrícula</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="511"/>
      <source> lines</source>
      <translation> línies</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="534"/>
      <source>Grid color and transparency</source>
      <translation>Colors i transparència dela Malla</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="554"/>
      <source>The color of the grid</source>
      <translation>El color de la graella</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="574"/>
      <source>The overall transparency of the grid</source>
      <translation>Transparència global de la graella</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="595"/>
      <source>Draft Edit preferences</source>
      <translation>Preferències d'edició d'esbós</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="598"/>
      <source>Edit</source>
      <translation>Edita</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="621"/>
      <source>Maximum number of contemporary edited objects</source>
      <translation>Nombre màxim d'objectes simultanis editats</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="644"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Sets the maximum number of objects Draft Edit&lt;/p&gt;&lt;p&gt;can process at the same time&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Estableix el nombre màxim d'objectes que Edició d'Esbós&lt;/p&gt;&lt;p&gt;pot processar al mateix temps&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="691"/>
      <source>Draft edit pick radius</source>
      <translation>Radi de selecció de l'edició d'Esbós</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="714"/>
      <source>Controls pick radius of edit nodes</source>
      <translation>Controla el radi de selecció dels nodes d’edició</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="14"/>
      <source>DWG</source>
      <translation>DWG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="26"/>
      <source>DWG conversion</source>
      <translation>Conversió DWG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="34"/>
      <source>Conversion method:</source>
      <translation>Mètode de conversió:</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="41"/>
      <source>This is the method FreeCAD will use to convert DWG files to DXF. If "Automatic" is chosen, FreeCAD will try to find one of the following converters in the same order as they are shown here. If FreeCAD is unable to find any, you might need to choose a specific converter and indicate its path here under. Choose the "dwg2dxf" utility if using LibreDWG, "ODAFileConverter" if using the ODA file converter, or the "dwg2dwg" utility if using the pro version of QCAD.</source>
      <translation type="unfinished">This is the method FreeCAD will use to convert DWG files to DXF. If "Automatic" is chosen, FreeCAD will try to find one of the following converters in the same order as they are shown here. If FreeCAD is unable to find any, you might need to choose a specific converter and indicate its path here under. Choose the "dwg2dxf" utility if using LibreDWG, "ODAFileConverter" if using the ODA file converter, or the "dwg2dwg" utility if using the pro version of QCAD.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="51"/>
      <source>Automatic</source>
      <translation>Automàtica</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="56"/>
      <source>LibreDWG</source>
      <translation>LibreDWG</translation>
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
      <translation>&lt;html&gt;&lt;head/&gt; &lt;body&gt;&lt;p&gt;&lt;span style="font-weight:600;"&gt; Nota:&lt;/span&gt; aplicar opcions DXF arxius DWG així.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="14"/>
      <source>User interface settings</source>
      <translation>Configuració de la interfície d'usuari</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="26"/>
      <source>In-Command Shortcuts</source>
      <translation>Dreceres d'ordres</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="37"/>
      <source>Relative</source>
      <translation>Relatiu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="59"/>
      <source>R</source>
      <translation>R</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="81"/>
      <source>Continue</source>
      <translation>Continua</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="103"/>
      <source>T</source>
      <translation>T</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="125"/>
      <source>Close</source>
      <translation>Tanca</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="147"/>
      <source>O</source>
      <translation>O</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="169"/>
      <source>Copy</source>
      <translation>Copia</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="191"/>
      <source>P</source>
      <translation>P</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="213"/>
      <source>Subelement Mode</source>
      <translation>Mode subelement</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="235"/>
      <source>D</source>
      <translation>D</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="257"/>
      <source>Fill</source>
      <translation>Ompli</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="279"/>
      <source>L</source>
      <translation>L</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="301"/>
      <source>Exit</source>
      <translation>Surt</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="323"/>
      <source>A</source>
      <translation>A</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="345"/>
      <source>Select Edge</source>
      <translation>Selecciona l'aresta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="367"/>
      <source>E</source>
      <translation>Afegir Columna</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="389"/>
      <source>Add Hold</source>
      <translation>Afegir Columna</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="411"/>
      <source>Q</source>
      <translation>Q</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="433"/>
      <source>Length</source>
      <translation>Longitud</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="455"/>
      <source>H</source>
      <translation>H</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="477"/>
      <source>Wipe</source>
      <translation>Neteja</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="499"/>
      <source>W</source>
      <translation>W</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="521"/>
      <source>Set WP</source>
      <translation>Definir el pla de treball</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="543"/>
      <source>U</source>
      <translation>U</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="565"/>
      <source>Cycle Snap</source>
      <translation>Cicle Snap</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="587"/>
      <source>`</source>
      <translation>`</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="609"/>
      <source>Global</source>
      <translation>Global</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="631"/>
      <source>G</source>
      <translation>G</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="653"/>
      <source>Snap</source>
      <translation>Ajusta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="675"/>
      <source>S</source>
      <translation>S</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="697"/>
      <source>Increase Radius</source>
      <translation>Augmenta el radi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="719"/>
      <source>[</source>
      <translation>[</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="741"/>
      <source>Decrease Radius</source>
      <translation>Disminueix el radi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="763"/>
      <source>]</source>
      <translation>]</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="785"/>
      <source>Restrict X</source>
      <translation>Restringeix X</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="807"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="829"/>
      <source>Restrict Y</source>
      <translation>Restringeix Y</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="851"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="873"/>
      <source>Restrict Z</source>
      <translation>Restricciò Z</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="895"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="928"/>
      <source>Enable draft statusbar customization</source>
      <translation>Activa la personalització de la barra d'estat d'esbós</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="931"/>
      <source>Draft Statusbar</source>
      <translation>Barra d'estat d'esbós</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="951"/>
      <source>Enable snap statusbar widget</source>
      <translation>Activa el giny d'ajust de la barra d'estat</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="954"/>
      <source>Draft snap widget</source>
      <translation>Giny d'ajust d'esbós</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="970"/>
      <source>Enable draft statusbar annotation scale widget</source>
      <translation>Activa el giny de l'anotació d'escala de la barra d'estat d'esbós</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="973"/>
      <source>Annotation scale widget</source>
      <translation>Giny d'escala d'anotació</translation>
    </message>
  </context>
  <context>
    <name>ImportAirfoilDAT</name>
    <message>
      <location filename="../../importAirfoilDAT.py" line="193"/>
      <source>Did not find enough coordinates</source>
      <translation type="unfinished">Did not find enough coordinates</translation>
    </message>
  </context>
  <context>
    <name>ImportSVG</name>
    <message>
      <location filename="../../importSVG.py" line="1796"/>
      <source>Unknown SVG export style, switching to Translated</source>
      <translation>Estil d'exportació SVG desconegut, s'està canviant a Traduït</translation>
    </message>
    <message>
      <location filename="../../importSVG.py" line="1816"/>
      <source>The export list contains no object with a valid bounding box</source>
      <translation>La llista d'exportació no conté cap objecte amb una caixa contenidora vàlida</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../InitGui.py" line="104"/>
      <source>Draft creation tools</source>
      <translation>Eines de creació d'Esbós</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="107"/>
      <source>Draft annotation tools</source>
      <translation>Eines d'anotació d'Esbós</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="110"/>
      <source>Draft modification tools</source>
      <translation>Eines de modificació d'Esbós</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="113"/>
      <source>Draft utility tools</source>
      <translation>Eines útils per a dibuixar</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="118"/>
      <source>&amp;Drafting</source>
      <translation>&amp;Borrador</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="121"/>
      <source>&amp;Annotation</source>
      <translation>&amp;Anotació</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="124"/>
      <source>&amp;Modification</source>
      <translation>&amp;Modificació</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="127"/>
      <source>&amp;Utilities</source>
      <translation>&amp;Utilitats</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="50"/>
      <source>Arc tools</source>
      <translation>Eines d'arc</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="58"/>
      <source>Bézier tools</source>
      <translation>Eines per a corbes Bézier</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="89"/>
      <source>Array tools</source>
      <translation>Eines de taules</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snapper.py" line="1477"/>
      <source>Draft Snap</source>
      <translation>Ajust de l'Esbós</translation>
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
      <translation>No s'han trobat en el sistema les llibreries d'importació/exportació DXF necessàries perquè FreeCAD pugui manegar el format DXF
Per permetre que FreeCAD descarregui aquestes llibreries:
 1 - Carregui el banc de treball Esbós
 2 - Menú Editar &gt; Preferències &gt; Importa-Exporta &gt; DXF &gt; Activa descàrregues
O descarregueu aquestes llibreries manualment tal com s'explica a
https://github.com/yorikvanhavre/Draft-dxf-importer
Per permetre que FreeCAD descarregui aquestes llibreries respongueu Sí.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="57"/>
      <location filename="../../DraftGui.py" line="751"/>
      <source>Relative</source>
      <translation>Relatiu</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="61"/>
      <location filename="../../DraftGui.py" line="756"/>
      <source>Global</source>
      <translation>Global</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="66"/>
      <location filename="../../DraftGui.py" line="774"/>
      <location filename="../../DraftGui.py" line="1126"/>
      <source>Continue</source>
      <translation>Continua</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="71"/>
      <location filename="../../DraftGui.py" line="790"/>
      <source>Close</source>
      <translation>Tanca</translation>
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
      <translation>Copia</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="81"/>
      <source>Subelement mode</source>
      <translation>Mode sub-element</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="86"/>
      <source>Fill</source>
      <translation>Ompli</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="91"/>
      <source>Exit</source>
      <translation>Surt</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="96"/>
      <source>Snap On/Off</source>
      <translation>Ajusta/desajusta</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="101"/>
      <source>Increase snap radius</source>
      <translation>Augmenta el radi d'ajust</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="106"/>
      <source>Decrease snap radius</source>
      <translation>Disminueix el radi d'ajust</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="111"/>
      <source>Restrict X</source>
      <translation>Restringeix X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="116"/>
      <source>Restrict Y</source>
      <translation>Restringeix Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="121"/>
      <source>Restrict Z</source>
      <translation>Restricciò Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="126"/>
      <location filename="../../DraftGui.py" line="796"/>
      <source>Select edge</source>
      <translation>Selecciona l'aresta</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="131"/>
      <source>Add custom snap point</source>
      <translation>Afegeix un punt d'ajust personalitzat</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="136"/>
      <source>Length mode</source>
      <translation>Mode de Llargària</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="141"/>
      <location filename="../../DraftGui.py" line="792"/>
      <source>Wipe</source>
      <translation>Neteja</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="146"/>
      <source>Set Working Plane</source>
      <translation>Estableix el pla de treball</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="151"/>
      <source>Cycle snap object</source>
      <translation>Canvia cíclicament l'ajust d'un objecte</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="156"/>
      <source>Toggle near snap on/off</source>
      <translation>Activa/desactiva el proper ajust</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="330"/>
      <source>Draft Command Bar</source>
      <translation>Barra d'ordres d'Esbós</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="659"/>
      <location filename="../../WorkingPlane.py" line="821"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="374"/>
      <source>Top</source>
      <translation>Planta</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="661"/>
      <location filename="../../WorkingPlane.py" line="832"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="386"/>
      <source>Front</source>
      <translation>Alçat</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="663"/>
      <location filename="../../WorkingPlane.py" line="843"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="398"/>
      <source>Side</source>
      <translation>Costat</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="665"/>
      <source>Auto</source>
      <translation>Auto</translation>
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
      <translation>Cap</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="728"/>
      <source>active command:</source>
      <translation>ordre actiu:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="730"/>
      <source>Active Draft command</source>
      <translation>Comanda activa d'esbós</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="731"/>
      <source>X coordinate of next point</source>
      <translation>X coordenada del punt següent</translation>
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
      <translation>Coordenada Y del punt següent</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="736"/>
      <source>Z coordinate of next point</source>
      <translation>Coordenada Z del punt següent</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="737"/>
      <source>Enter point</source>
      <translation>Entri el punt</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="739"/>
      <source>Enter a new point with the given coordinates</source>
      <translation>Subministri un nou punt amb les coordenades donades</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="740"/>
      <source>Length</source>
      <translation>Longitud</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="741"/>
      <location filename="../../draftguitools/gui_trimex.py" line="220"/>
      <source>Angle</source>
      <translation>Angle</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="742"/>
      <source>Length of current segment</source>
      <translation>Longitud del segment actual</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="743"/>
      <source>Angle of current segment</source>
      <translation>Angle del segment actual</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="747"/>
      <source>Check this to lock the current angle</source>
      <translation>Marqueu aquesta casella per a blocar l'angle actual</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="748"/>
      <location filename="../../DraftGui.py" line="1108"/>
      <source>Radius</source>
      <translation>Radi</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="749"/>
      <location filename="../../DraftGui.py" line="1109"/>
      <source>Radius of Circle</source>
      <translation>Radi del cercle</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="754"/>
      <source>Coordinates relative to last point or to coordinate system origin
if is the first point to set</source>
      <translation type="unfinished">Coordinates relative to last point or to coordinate system origin
if is the first point to set</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="759"/>
      <source>Coordinates relative to global coordinate system.
Uncheck to use working plane coordinate system</source>
      <translation type="unfinished">Coordinates relative to global coordinate system.
Uncheck to use working plane coordinate system</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="761"/>
      <source>Filled</source>
      <translation>Emplenat</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="765"/>
      <source>Check this if the object should appear as filled, otherwise it will appear as wireframe.
Not available if Draft preference option 'Use Part Primitives' is enabled</source>
      <translation type="unfinished">Check this if the object should appear as filled, otherwise it will appear as wireframe.
Not available if Draft preference option 'Use Part Primitives' is enabled</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="767"/>
      <source>Finish</source>
      <translation>Finalitza</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="769"/>
      <source>Finishes the current drawing or editing operation</source>
      <translation>Acaba el dibuix actual o l'operació d'edició</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="772"/>
      <source>If checked, command will not finish until you press the command button again</source>
      <translation>Si està marcada, comanda no acabarà fins que es premi el botó de comanda una altra vegada</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="777"/>
      <source>If checked, an OCC-style offset will be performedinstead of the classic offset</source>
      <translation type="unfinished">If checked, an OCC-style offset will be performedinstead of the classic offset</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="778"/>
      <source>&amp;OCC-style offset</source>
      <translation>Desplaçament de l'estil &amp;OCC</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="788"/>
      <source>&amp;Undo (CTRL+Z)</source>
      <translation>&amp;Desfés (CTRL+Z)</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="789"/>
      <source>Undo the last segment</source>
      <translation>Desfés l'últim segment</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="791"/>
      <source>Finishes and closes the current line</source>
      <translation>Finalitza i tanca la línia actual</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="793"/>
      <source>Wipes the existing segments of this line and starts again from the last point</source>
      <translation>Esborra els segments existents d'aquesta línia i comença de nou desde l'últim punt</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="794"/>
      <source>Set WP</source>
      <translation>Definir el pla de treball</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="795"/>
      <source>Reorients the working plane on the last segment</source>
      <translation>Reorienta el pla de treball de l'últim segment</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="797"/>
      <source>Selects an existing edge to be measured by this dimension</source>
      <translation>Selecciona una aresta existent per a mesurar-la amb aquesta dimensió</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="798"/>
      <source>Sides</source>
      <translation>Costats</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="799"/>
      <source>Number of sides</source>
      <translation>Nombre de costats</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="802"/>
      <source>If checked, objects will be copied instead of moved. Preferences -&gt; Draft -&gt; Global copy mode to keep this mode in next commands</source>
      <translation>Si està activada, els objectes es copiaran en lloc de moure's. Preferències -&gt; Esbós -&gt; Mode de còpia global per a mantenir aquest mode en les ordres següents</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="803"/>
      <source>Modify subelements</source>
      <translation>Modifica els sub-elements</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="804"/>
      <source>If checked, subelements will be modified instead of entire objects</source>
      <translation>Si està marcat, els sub-elements es modificaran en lloc dels objectes sencers</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="805"/>
      <source>Text string to draw</source>
      <translation>Cadena de text a dibuixar</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="806"/>
      <source>String</source>
      <translation>Cadena</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="807"/>
      <source>Height of text</source>
      <translation>Alçada del text</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="808"/>
      <source>Height</source>
      <translation>Alçària</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="809"/>
      <source>Intercharacter spacing</source>
      <translation>Espaiat de Inter-caràcter</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="810"/>
      <source>Tracking</source>
      <translation>Seguiment</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="811"/>
      <source>Full path to font file:</source>
      <translation>Ruta d'acces completa al arxiu font:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="812"/>
      <source>Open a FileChooser for font file</source>
      <translation>Obrir un selector de fitxers per a arxiu de font</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="813"/>
      <source>Create text</source>
      <translation>Crear text</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="814"/>
      <source>Press this button to create the text object, or finish your text with two blank lines</source>
      <translation>Premeu aquest botó per a crear l’objecte de text o finalitzeu el vostre text amb dues línies en blanc</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="836"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="272"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="327"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="530"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="541"/>
      <source>Current working plane</source>
      <translation>Pla de treball actual</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="837"/>
      <source>Change default style for new objects</source>
      <translation>Canvia l'estil per defecte dels objectes nous</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="838"/>
      <source>Toggle construction mode</source>
      <translation>Activar/desactivar el mode de construcció</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="839"/>
      <location filename="../../DraftGui.py" line="2050"/>
      <location filename="../../DraftGui.py" line="2065"/>
      <source>Autogroup off</source>
      <translation>Desactivar Auto-agrupar</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="950"/>
      <source>Line</source>
      <translation>Línia</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="958"/>
      <source>DWire</source>
      <translation>DWire</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="976"/>
      <source>Circle</source>
      <translation>Cercle</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="981"/>
      <source>Arc</source>
      <translation>Arc</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="986"/>
      <location filename="../../draftguitools/gui_rotate.py" line="286"/>
      <source>Rotate</source>
      <translation>Rotació</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="990"/>
      <source>Point</source>
      <translation>Punt</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1018"/>
      <source>Label</source>
      <translation>Etiqueta</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1036"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="527"/>
      <location filename="../../draftguitools/gui_offset.py" line="243"/>
      <location filename="../../draftguitools/gui_offset.py" line="260"/>
      <location filename="../../draftguitools/gui_offset.py" line="324"/>
      <source>Offset</source>
      <translation>Equidistancia (ofset)</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1042"/>
      <location filename="../../DraftGui.py" line="1100"/>
      <location filename="../../draftguitools/gui_trimex.py" line="215"/>
      <source>Distance</source>
      <translation type="unfinished">Distance</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1043"/>
      <location filename="../../DraftGui.py" line="1101"/>
      <location filename="../../draftguitools/gui_trimex.py" line="217"/>
      <source>Offset distance</source>
      <translation>Distància de separació</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1097"/>
      <source>Trimex</source>
      <translation>Trimex</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1197"/>
      <source>Pick Object</source>
      <translation>Designar objecte</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1203"/>
      <source>Edit</source>
      <translation>Edita</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1253"/>
      <source>Local u0394X</source>
      <translation>u0394X local</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1254"/>
      <source>Local u0394Y</source>
      <translation>u0394Y local</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1255"/>
      <source>Local u0394Z</source>
      <translation>u0394Z local</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1257"/>
      <source>Local X</source>
      <translation>Local X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1258"/>
      <source>Local Y</source>
      <translation>Local Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1259"/>
      <source>Local Z</source>
      <translation>Local Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1261"/>
      <source>Global u0394X</source>
      <translation>u0394X global</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1262"/>
      <source>Global u0394Y</source>
      <translation>u0394Y global</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1263"/>
      <source>Global u0394Z</source>
      <translation>u0394Z global</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1265"/>
      <source>Global X</source>
      <translation>Global X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1266"/>
      <source>Global Y</source>
      <translation>Global Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1267"/>
      <source>Global Z</source>
      <translation>Global Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1503"/>
      <source>Invalid Size value. Using 200.0.</source>
      <translation>Valor de mida no vàlid. Utilitzant 200.0.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1511"/>
      <source>Invalid Tracking value. Using 0.</source>
      <translation>Valor de seguiment no és vàlid. Utilitzant 0.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1525"/>
      <source>Please enter a text string.</source>
      <translation>Si us plau, introduïu una cadena de text.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1534"/>
      <source>Select a Font file</source>
      <translation>Seleccioneu un fitxer de tipus de lletra</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1567"/>
      <source>Please enter a font file.</source>
      <translation>Si us plau, introduïu un fitxer de tipus de lletra.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2058"/>
      <source>Autogroup:</source>
      <translation>Agrupament automàtic:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2394"/>
      <source>Faces</source>
      <translation>Cares</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2395"/>
      <source>Remove</source>
      <translation>Elimina</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2396"/>
      <source>Add</source>
      <translation>Afegeix</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2397"/>
      <source>Facebinder elements</source>
      <translation>Elements del Lligacares</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="46"/>
      <source>Draft</source>
      <translation>Calat</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="209"/>
      <location filename="../../importDWG.py" line="281"/>
      <source>LibreDWG error</source>
      <translation type="unfinished">LibreDWG error</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="218"/>
      <location filename="../../importDWG.py" line="290"/>
      <source>Converting:</source>
      <translation type="unfinished">Converting:</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="223"/>
      <source>Conversion successful</source>
      <translation>Conversió correcta</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="226"/>
      <source>Error during DWG conversion. Try moving the DWG file to a directory path without spaces and non-english characters, or try saving to a lower DWG version.</source>
      <translation type="unfinished">Error during DWG conversion. Try moving the DWG file to a directory path without spaces and non-english characters, or try saving to a lower DWG version.</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="229"/>
      <location filename="../../importDWG.py" line="296"/>
      <source>ODA File Converter not found</source>
      <translation type="unfinished">ODA File Converter not found</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="242"/>
      <location filename="../../importDWG.py" line="306"/>
      <source>QCAD error</source>
      <translation type="unfinished">QCAD error</translation>
    </message>
    <message>
      <location filename="../../WorkingPlane.py" line="713"/>
      <location filename="../../draftmake/make_sketch.py" line="127"/>
      <location filename="../../draftmake/make_sketch.py" line="139"/>
      <source>All Shapes must be coplanar</source>
      <translation type="unfinished">All Shapes must be coplanar</translation>
    </message>
    <message>
      <location filename="../../WorkingPlane.py" line="721"/>
      <source>Selected Shapes must define a plane</source>
      <translation type="unfinished">Selected Shapes must define a plane</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="81"/>
      <source>No graphical interface</source>
      <translation type="unfinished">No graphical interface</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="161"/>
      <source>Unable to insert new object into a scaled part</source>
      <translation>No es pot inserir un nou objecte a una part escalada</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="267"/>
      <source>Symbol not implemented. Using a default symbol.</source>
      <translation type="unfinished">Symbol not implemented. Using a default symbol.</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="333"/>
      <source>Visibility off; removed from list: </source>
      <translation type="unfinished">Visibility off; removed from list: </translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="603"/>
      <source>image is Null</source>
      <translation type="unfinished">image is Null</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="609"/>
      <source>filename does not exist on the system or in the resource file</source>
      <translation type="unfinished">filename does not exist on the system or in the resource file</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="668"/>
      <source>unable to load texture</source>
      <translation type="unfinished">unable to load texture</translation>
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
      <translation>Cap document actiu. S'està interrompent.</translation>
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
      <translation>Entrada incorrecta: l'objecte no és al document.</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="738"/>
      <source>Does not have 'ViewObject.RootNode'.</source>
      <translation type="unfinished">Does not have 'ViewObject.RootNode'.</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="51"/>
      <location filename="../../draftutils/init_draft_statusbar.py" line="58"/>
      <location filename="../../draftutils/init_draft_statusbar.py" line="65"/>
      <location filename="../../draftutils/init_draft_statusbar.py" line="154"/>
      <source>custom</source>
      <translation>personalitzat</translation>
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
      <translation>Estableix l'escala personalitzada de l'anotació amb el format x:x, x=x</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="214"/>
      <source>Set the scale used by draft annotation tools</source>
      <translation type="unfinished">Set the scale used by draft annotation tools</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="650"/>
      <source>Solids:</source>
      <translation type="unfinished">Solids:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="651"/>
      <source>Faces:</source>
      <translation>Cares:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="652"/>
      <source>Wires:</source>
      <translation type="unfinished">Wires:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="653"/>
      <source>Edges:</source>
      <translation type="unfinished">Edges:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="654"/>
      <source>Vertices:</source>
      <translation type="unfinished">Vertices:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="658"/>
      <source>Face</source>
      <translation>Cara</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="663"/>
      <source>Wire</source>
      <translation>Cable</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="695"/>
      <location filename="../../draftutils/utils.py" line="699"/>
      <source>different types</source>
      <translation type="unfinished">different types</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="709"/>
      <source>Objects have different placements. Distance between the two base points: </source>
      <translation type="unfinished">Objects have different placements. Distance between the two base points: </translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="712"/>
      <source>has a different value</source>
      <translation type="unfinished">has a different value</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="715"/>
      <source>doesn't exist in one of the objects</source>
      <translation type="unfinished">doesn't exist in one of the objects</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="827"/>
      <source>%s shares a base with %d other objects. Please check if you want to modify this.</source>
      <translation type="unfinished">%s shares a base with %d other objects. Please check if you want to modify this.</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="833"/>
      <source>%s cannot be modified because its placement is readonly.</source>
      <translation type="unfinished">%s cannot be modified because its placement is readonly.</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="977"/>
      <source>Wrong input: unknown document.</source>
      <translation type="unfinished">Wrong input: unknown document.</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="1055"/>
      <source>This function will be deprecated in </source>
      <translation type="unfinished">This function will be deprecated in </translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="1056"/>
      <location filename="../../draftutils/utils.py" line="1059"/>
      <source>Please use </source>
      <translation type="unfinished">Please use </translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="1059"/>
      <source>This function will be deprecated. </source>
      <translation type="unfinished">This function will be deprecated. </translation>
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
      <translation>Commuta la graella d'Esbós</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="69"/>
      <source>ShapeString: string has no wires</source>
      <translation>Text-forma: la cadena no té filferros</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="89"/>
      <location filename="../../draftobjects/draft_annotation.py" line="105"/>
      <source>added view property 'ScaleMultiplier'</source>
      <translation>s'ha afegit la propietat de vista 'ScaleMultiplier'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="125"/>
      <location filename="../../draftobjects/draft_annotation.py" line="130"/>
      <source>migrated 'DraftText' type to 'Text'</source>
      <translation>s'ha canviat el tipus 'Text d'Esbós' a 'Text'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="284"/>
      <source>, path object doesn't have 'Edges'.</source>
      <translation>, l'objecte camí no té 'Arestes'.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="395"/>
      <location filename="../../draftobjects/patharray.py" line="401"/>
      <location filename="../../draftobjects/patharray.py" line="407"/>
      <source>'PathObj' property will be migrated to 'PathObject'</source>
      <translation>la propietat 'PathObj' es canviarà per 'PathObject'</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="525"/>
      <source>Cannot calculate path tangent. Copy not aligned.</source>
      <translation>No es pot calcular la tangent del camí. La còpia no està alineada.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="541"/>
      <source>Tangent and normal are parallel. Copy not aligned.</source>
      <translation>La tangent i la normal són paral·leles. La còpia no està alineada.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="558"/>
      <source>Cannot calculate path normal, using default.</source>
      <translation>No es pot calcular la normal del camí, s'utilitzarà el valor per defecte.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="565"/>
      <source>Cannot calculate path binormal. Copy not aligned.</source>
      <translation>No es pot calcular la binormal del camí. La còpia no està alineada.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="571"/>
      <source>AlignMode {} is not implemented</source>
      <translation>AlignMode{} no està implementat</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="145"/>
      <location filename="../../draftobjects/pointarray.py" line="161"/>
      <source>added property 'ExtraPlacement'</source>
      <translation>s'ha afegit la propietat 'ExtraPlacement'</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="151"/>
      <source>Object must be a closed shape</source>
      <translation>L'objecte ha de ser una forma tancada</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="153"/>
      <source>No solid object created</source>
      <translation>No s'ha creat cap objecte sòlid</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="276"/>
      <source>Faces must be coplanar to be refined</source>
      <translation>Les cares han de ser coplanars per poder refinar-les</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="435"/>
      <location filename="../../draftfunctions/downgrade.py" line="230"/>
      <source>Upgrade: Unknown force method:</source>
      <translation>Actualització: Mètode de força desconegut:</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="453"/>
      <source>Found groups: closing each open object inside</source>
      <translation>S'han trobat grups: s'esta tancant cada objecte de dins</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="459"/>
      <source>Found meshes: turning into Part shapes</source>
      <translation>S'han trobat malles: s'estan convertint en formes de Peça</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="467"/>
      <source>Found 1 solidifiable object: solidifying it</source>
      <translation>S'ha trobat un objecte solidificable: s'està solidificant</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="472"/>
      <source>Found 2 objects: fusing them</source>
      <translation>S'han trobat 2 objectes: s'estan fusionant</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="483"/>
      <source>Found object with several coplanar faces: refine them</source>
      <translation>S'ha trobat un objecte amb diverses cares coplanars: refineu-les</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="489"/>
      <source>Found 1 non-parametric objects: draftifying it</source>
      <translation>S'ha trobat 1 objecte no paramètric: s'està esbossant</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="500"/>
      <source>Found 1 closed sketch object: creating a face from it</source>
      <translation>S'ha trobat 1 objecte croquis tancat: es crearà una cara</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="505"/>
      <source>Found closed wires: creating faces</source>
      <translation>S'han trobat fils tancats: se'n creen cares</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="511"/>
      <source>Found several wires or edges: wiring them</source>
      <translation>S'han trobat diversos filferros o arestes: s'uniran</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="513"/>
      <location filename="../../draftfunctions/upgrade.py" line="547"/>
      <source>Found several non-treatable objects: creating compound</source>
      <translation>S'han trobat diversos objectes intractables: s'està creant un compost</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="518"/>
      <source>trying: closing it</source>
      <translation>provant: s'està tancant</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="520"/>
      <source>Found 1 open wire: closing it</source>
      <translation>S'ha trobat un filferro obert: es tancarà</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="537"/>
      <source>Found 1 object: draftifying it</source>
      <translation>Trobat un objecte: esbossant-lo</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="542"/>
      <source>Found points: creating compound</source>
      <translation>S'han trobat punts: s'està creant un compost</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="550"/>
      <source>Unable to upgrade these objects.</source>
      <translation>No es poden actualitzar aquests objectes.</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="90"/>
      <source>No object given</source>
      <translation>No s'ha proporcionat cap objecte</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="94"/>
      <source>The two points are coincident</source>
      <translation>Els dos punts coincideixen</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="113"/>
      <source>mirrored</source>
      <translation type="unfinished">mirrored</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="238"/>
      <source>Found 1 block: exploding it</source>
      <translation>S'ha trobat 1 bloc: s'està explotant</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="246"/>
      <source>Found 1 multi-solids compound: exploding it</source>
      <translation>S'ha trobat 1 compost multi-sòlids: s'està explotant</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="253"/>
      <source>Found 1 parametric object: breaking its dependencies</source>
      <translation>S'ha trobat 1 objecte paramètric: s'estan trencant les seves dependències</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="261"/>
      <source>Found 2 objects: subtracting them</source>
      <translation>S'han trobat 2 objectes: s'estan restant</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="268"/>
      <source>Found several faces: splitting them</source>
      <translation>S'han trobat diverses cares: s'estan dividint</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="273"/>
      <source>Found several objects: subtracting them from the first one</source>
      <translation>S'han trobat diversos objectes: s'estan restant del primer</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="278"/>
      <source>Found 1 face: extracting its wires</source>
      <translation>S'ha trobat 1 cara: se n'extreuen els filferros</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="284"/>
      <source>Found only wires: extracting their edges</source>
      <translation>Tansols s'han trobat filferros: se n'extreuen les arestes</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="288"/>
      <source>No more downgrade possible</source>
      <translation>No es pot degradar més</translation>
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
      <translation>Entrada incorrecta: ha de ser un vector.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_pointarray.py" line="147"/>
      <location filename="../../draftmake/make_text.py" line="107"/>
      <location filename="../../draftmake/make_label.py" line="215"/>
      <source>Wrong input: must be a placement, a vector, or a rotation.</source>
      <translation>Entrada incorrecta: ha de ser un posicionament, un vector o una rotació.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="316"/>
      <location filename="../../draftmake/make_label.py" line="230"/>
      <source>Wrong input: object must not be a list.</source>
      <translation>Entrada incorrecta: l'objecte no ha de ser una llista.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_patharray.py" line="213"/>
      <location filename="../../draftmake/make_label.py" line="251"/>
      <source>Wrong input: must be a list or tuple of strings, or a single string.</source>
      <translation>Entrada incorrecta: ha de ser una llista o tupla de cadenes, o una sola cadena.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="263"/>
      <source>Wrong input: subelement not in object.</source>
      <translation>Entrada incorrecta: el subelement no és a l'objecte.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="272"/>
      <source>Wrong input: label_type must be a string.</source>
      <translation type="unfinished">Wrong input: label_type must be a string.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="277"/>
      <source>Wrong input: label_type must be one of the following: </source>
      <translation type="unfinished">Wrong input: label_type must be one of the following: </translation>
    </message>
    <message>
      <location filename="../../draftmake/make_text.py" line="91"/>
      <location filename="../../draftmake/make_text.py" line="96"/>
      <location filename="../../draftmake/make_label.py" line="286"/>
      <location filename="../../draftmake/make_label.py" line="291"/>
      <source>Wrong input: must be a list of strings or a single string.</source>
      <translation>Entrada incorrecta: ha de ser una llista de cadenes o una sola cadena.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="300"/>
      <location filename="../../draftmake/make_label.py" line="304"/>
      <source>Wrong input: must be a string, 'Horizontal', 'Vertical', or 'Custom'.</source>
      <translation>Entrada incorrecta: ha de ser una cadena, 'Horitzontal', 'Vertical' o 'Personalitzat'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_polararray.py" line="119"/>
      <location filename="../../draftmake/make_layer.py" line="201"/>
      <location filename="../../draftmake/make_patharray.py" line="191"/>
      <location filename="../../draftmake/make_patharray.py" line="360"/>
      <location filename="../../draftmake/make_orthoarray.py" line="151"/>
      <location filename="../../draftmake/make_label.py" line="313"/>
      <source>Wrong input: must be a number.</source>
      <translation>Entrada incorrecta: ha de ser un nombre.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="320"/>
      <source>Wrong input: must be a list of at least two vectors.</source>
      <translation>Entrada incorrecta: ha de ser una llista de dos vectors com a mínim.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="353"/>
      <source>Direction is not 'Custom'; points won't be used.</source>
      <translation>La direcció no és 'Personalitzada'; no s'usaran els punts.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="380"/>
      <source>Wrong input: must be a list of two elements. For example, [object, 'Edge1'].</source>
      <translation>Entrada incorrecta: ha de ser una llista de dos elements. Per exemple, [objecte, 'Aresta1'].</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_pointarray.py" line="135"/>
      <source>Wrong input: point object doesn't have 'Geometry', 'Links', or 'Components'.</source>
      <translation>Entrada incorrecta: l'objecte punt no té 'Geometria', 'Enllaços', o 'Components'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="69"/>
      <source>Layers</source>
      <translation>Capes</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="145"/>
      <location filename="../../draftmake/make_layer.py" line="162"/>
      <location filename="../../draftguitools/gui_layers.py" line="47"/>
      <source>Layer</source>
      <translation>Capa</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="157"/>
      <source>Wrong input: it must be a string.</source>
      <translation>Entrada incorrecta: ha de ser una cadena.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="167"/>
      <location filename="../../draftmake/make_layer.py" line="171"/>
      <location filename="../../draftmake/make_layer.py" line="184"/>
      <location filename="../../draftmake/make_layer.py" line="188"/>
      <source>Wrong input: must be a tuple of three floats 0.0 to 1.0.</source>
      <translation>Entrada incorrecta: ha de ser una tupla de tres nombres decimals entre 0.0 i 1.0.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="208"/>
      <location filename="../../draftmake/make_layer.py" line="212"/>
      <source>Wrong input: must be 'Solid', 'Dashed', 'Dotted', or 'Dashdot'.</source>
      <translation>Entrada incorrecta: ha de ser 'Solid', 'Dashed', 'Dotted', o 'Dashdot'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="220"/>
      <source>Wrong input: must be a number between 0 and 100.</source>
      <translation>Entrada incorrecta: ha de ser un nombre entre 0 i 100.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="173"/>
      <source>This function is deprecated. Do not use this function directly.</source>
      <translation>Aquesta funció és obsoleta. No la useu directament.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="174"/>
      <source>Use one of 'make_linear_dimension', or 'make_linear_dimension_obj'.</source>
      <translation>Useu 'make_linear_dimension' o bé 'make_linear_dimension_obj'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="327"/>
      <location filename="../../draftmake/make_dimension.py" line="452"/>
      <source>Wrong input: object doesn't have a 'Shape' to measure.</source>
      <translation>Entrada incorrecta: no hi ha cap 'Forma' per mesurar.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="331"/>
      <source>Wrong input: object doesn't have at least one element in 'Vertexes' to use for measuring.</source>
      <translation>Entrada incorrecta: l'objecte no té com a mínim un element a 'Vèrtexs' útil per mesurar.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="338"/>
      <location filename="../../draftmake/make_dimension.py" line="463"/>
      <source>Wrong input: must be an integer.</source>
      <translation>Entrada incorrecta: ha de ser un enter.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="343"/>
      <source>i1: values below 1 are not allowed; will be set to 1.</source>
      <translation>i1: no es permetenn els valors inferiors a 1; s'estableix el valor a 1.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="347"/>
      <location filename="../../draftmake/make_dimension.py" line="363"/>
      <source>Wrong input: vertex not in object.</source>
      <translation>Entrada incorrecta: el vèrtex no és a l'objecte.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="359"/>
      <source>i2: values below 1 are not allowed; will be set to the last vertex in the object.</source>
      <translation>i2: no es permeten valors inferiors a 1; s'entrarà l'últim vèrtex de l'objecte.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="456"/>
      <source>Wrong input: object doesn't have at least one element in 'Edges' to use for measuring.</source>
      <translation>Entrada incorrecta: l'objecte no té com a mínim un element a 'Arestes' útil per mesurar.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="468"/>
      <source>index: values below 1 are not allowed; will be set to 1.</source>
      <translation>índex: no es permetenn els valors inferiors a 1; s'estableix el valor a 1.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="472"/>
      <source>Wrong input: index doesn't correspond to an edge in the object.</source>
      <translation>Entrada incorrecta: l'índex no correspon a una aresta de l'objecte.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="476"/>
      <source>Wrong input: index doesn't correspond to a circular edge.</source>
      <translation>Entrada incorrecta: l'índex no correspon a una aresta circular.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="483"/>
      <location filename="../../draftmake/make_dimension.py" line="487"/>
      <source>Wrong input: must be a string, 'radius' or 'diameter'.</source>
      <translation>Entrada incorrecta: ha de ser una cadena, 'radi' o 'diàmetre'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="579"/>
      <location filename="../../draftmake/make_dimension.py" line="586"/>
      <source>Wrong input: must be a list with two angles.</source>
      <translation>Entrada incorrecta: ha de ser una llista de dos angles.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="60"/>
      <source>Internal orthogonal array</source>
      <translation>Matriu ortogonal interna</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="87"/>
      <source>Wrong input: must be a number or vector.</source>
      <translation>Entrada incorrecta: ha de ser un nombre o un vector.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="92"/>
      <location filename="../../draftmake/make_orthoarray.py" line="95"/>
      <location filename="../../draftmake/make_orthoarray.py" line="98"/>
      <source>Input: single value expanded to vector.</source>
      <translation>Entrada: un valor únic s'ha convertit en vector.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="154"/>
      <location filename="../../draftmake/make_polararray.py" line="112"/>
      <location filename="../../draftmake/make_orthoarray.py" line="119"/>
      <source>Wrong input: must be an integer number.</source>
      <translation>Entrada incorrecta: ha de ser un nombre enter.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="123"/>
      <location filename="../../draftmake/make_orthoarray.py" line="126"/>
      <location filename="../../draftmake/make_orthoarray.py" line="129"/>
      <source>Input: number of elements must be at least 1. It is set to 1.</source>
      <translation>Entrada: el nombre d'elements ha de ser com a mínim 1. S'estableix a 1.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="275"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="269"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="396"/>
      <source>Orthogonal array</source>
      <translation>Matriu ortogonal</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="350"/>
      <source>Orthogonal array 2D</source>
      <translation>Matriu ortogonal 2D</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="424"/>
      <source>Rectangular array</source>
      <translation>Matriu rectangular</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="501"/>
      <source>Rectangular array 2D</source>
      <translation>Matriu rectangular 2D</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_polararray.py" line="94"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="258"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="434"/>
      <source>Polar array</source>
      <translation>Matriu polar</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_patharray.py" line="246"/>
      <source>Wrong input: must be 'Original', 'Frenet', or 'Tangent'.</source>
      <translation>Entrada incorrecta: ha de ser 'Original', 'Frenet', o 'Tangent'.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="125"/>
      <location filename="../../draftmake/make_arc_3points.py" line="130"/>
      <source>Points:</source>
      <translation>Punts:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="126"/>
      <location filename="../../draftmake/make_arc_3points.py" line="131"/>
      <source>Wrong input: must be list or tuple of three points exactly.</source>
      <translation>Entrada incorrecta: ha de ser una llista o una tupla de tres punts exactament.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="138"/>
      <source>Placement:</source>
      <translation>Ubicació:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="139"/>
      <source>Wrong input: incorrect type of placement.</source>
      <translation>Entrada incorrecta: tipus de posicionament incorrecte.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="153"/>
      <source>Wrong input: incorrect type of points.</source>
      <translation>Entrada incorrecta: tipus de punts incorrectes.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="159"/>
      <source>Cannot generate shape:</source>
      <translation type="unfinished">Cannot generate shape:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="166"/>
      <source>Radius:</source>
      <translation>Radi:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="167"/>
      <source>Center:</source>
      <translation>Centre:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="170"/>
      <source>Create primitive object</source>
      <translation>Crea un objecte primitiva</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="193"/>
      <location filename="../../draftmake/make_arc_3points.py" line="204"/>
      <source>Final placement:</source>
      <translation>Posicionament final:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="195"/>
      <source>Face: True</source>
      <translation>Cara: Cert</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="197"/>
      <source>Support:</source>
      <translation>Suport:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="198"/>
      <source>Map mode:</source>
      <translation>Mode de mapat:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_sketch.py" line="104"/>
      <source>No shape found</source>
      <translation type="unfinished">No shape found</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_sketch.py" line="111"/>
      <source>All Shapes must be planar</source>
      <translation type="unfinished">All Shapes must be planar</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="122"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="290"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="481"/>
      <source>Circular array</source>
      <translation>Matriu circular</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="144"/>
      <source>Wrong input: must be a number or quantity.</source>
      <translation>Entrada incorrecta: ha de ser un nombre o una quantitat.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="58"/>
      <location filename="../../draftmake/make_fillet.py" line="143"/>
      <location filename="../../draftmake/make_fillet.py" line="144"/>
      <location filename="../../draftmake/make_fillet.py" line="145"/>
      <source>length:</source>
      <translation>longitud:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="132"/>
      <source>Two elements are needed.</source>
      <translation>Calen dos elements.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="139"/>
      <source>Radius is too large</source>
      <translation>Radi massa gran</translation>
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
      <translation>Els objectes d'origen s'han eliminat.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="87"/>
      <source>Select an object to scale</source>
      <translation>Seleccioneu un objecte per escalar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="108"/>
      <source>Pick base point</source>
      <translation>Tria el punt de base</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="135"/>
      <source>Pick reference distance from base point</source>
      <translation>Trieu una distància de referència des del punt de base</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="206"/>
      <location filename="../../draftguitools/gui_scale.py" line="236"/>
      <location filename="../../draftguitools/gui_scale.py" line="359"/>
      <source>Scale</source>
      <translation>Escala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="209"/>
      <source>Some subelements could not be scaled.</source>
      <translation>Alguns subelements no s'han pogut escalar.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="339"/>
      <source>Unable to scale object:</source>
      <translation>No es pot escalar l'objecte:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="343"/>
      <source>Unable to scale objects:</source>
      <translation>No es poden escalar els objectes:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="346"/>
      <source>This object type cannot be scaled directly. Please use the clone method.</source>
      <translation>Aquest tipus d'objecte no es pot escalar directament. Si us plau, useu el mètode de clonació.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="407"/>
      <source>Pick new distance from base point</source>
      <translation>Trieu una nova distància des del punt de base</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_sketcher_objects.py" line="63"/>
      <source>Sketch is too complex to edit: it is suggested to use sketcher default editor</source>
      <translation>El croquis és massa complex per editar-lo: es suggereix usar l'editor per defecte de croquis</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="80"/>
      <source>Pick target point</source>
      <translation>Seleccioni el punt destí</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="157"/>
      <source>Create Label</source>
      <translation>Crea etiqueta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="191"/>
      <location filename="../../draftguitools/gui_labels.py" line="218"/>
      <source>Pick endpoint of leader line</source>
      <translation>Trieu el punt final de la línia guia</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="201"/>
      <location filename="../../draftguitools/gui_labels.py" line="228"/>
      <source>Pick text position</source>
      <translation>Seleccioni la posició del text</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="75"/>
      <source>Change Style</source>
      <translation>Canvia l'estil</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="77"/>
      <source>Pick location point</source>
      <translation>Trieu punt d'ubicació</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="121"/>
      <source>Create Text</source>
      <translation>Crea un text</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="51"/>
      <source>Toggle grid</source>
      <translation>Commuta la graella</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="150"/>
      <source>Pick a face, 3 vertices or a WP Proxy to define the drawing plane</source>
      <translation>Trieu una cara, 3 vèrtexs o un Proxi de PT per definir el pla del dibuix</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="274"/>
      <source>Working plane aligned to global placement of</source>
      <translation>El pla de treball s'ha alineat al posicionament global de</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="523"/>
      <source>Dir</source>
      <translation>Dir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="539"/>
      <source>Custom</source>
      <translation>Personalitzat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="71"/>
      <source>Select faces from existing objects</source>
      <translation>Selecciona cares d'objectes existents</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="73"/>
      <source>Select an object to mirror</source>
      <translation>Seleccioneu un objecte per a reflectir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="92"/>
      <source>Pick start point of mirror line</source>
      <translation>Trieu el punt de partida de la línia de reflexió</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="122"/>
      <source>Mirror</source>
      <translation>Reflexa</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="177"/>
      <location filename="../../draftguitools/gui_mirror.py" line="203"/>
      <source>Pick end point of mirror line</source>
      <translation>Trieu el punt final de la línia de reflexió</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="78"/>
      <location filename="../../draftguitools/gui_arcs.py" line="88"/>
      <source>Pick center point</source>
      <translation>Trieu el punt central</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="189"/>
      <location filename="../../draftguitools/gui_polygons.py" line="200"/>
      <location filename="../../draftguitools/gui_polygons.py" line="260"/>
      <location filename="../../draftguitools/gui_arcs.py" line="254"/>
      <location filename="../../draftguitools/gui_arcs.py" line="270"/>
      <location filename="../../draftguitools/gui_arcs.py" line="410"/>
      <source>Pick radius</source>
      <translation>Trieu el radi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="277"/>
      <location filename="../../draftguitools/gui_arcs.py" line="278"/>
      <location filename="../../draftguitools/gui_arcs.py" line="446"/>
      <location filename="../../draftguitools/gui_arcs.py" line="447"/>
      <source>Start angle</source>
      <translation>Angle inicial</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="283"/>
      <location filename="../../draftguitools/gui_arcs.py" line="452"/>
      <source>Pick start angle</source>
      <translation>Trieu l'angle d'inici</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="285"/>
      <location filename="../../draftguitools/gui_arcs.py" line="286"/>
      <location filename="../../draftguitools/gui_arcs.py" line="454"/>
      <location filename="../../draftguitools/gui_arcs.py" line="455"/>
      <source>Aperture angle</source>
      <translation>Angle d'obertura</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="291"/>
      <source>Pick aperture</source>
      <translation>Trieu obertura</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="317"/>
      <source>Create Circle (Part)</source>
      <translation>Crea Cercle (Peça)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="335"/>
      <source>Create Circle</source>
      <translation>Crea una circumferència</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="369"/>
      <source>Create Arc (Part)</source>
      <translation>Crea Arc (Peça)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="389"/>
      <source>Create Arc</source>
      <translation>Crea un arc</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="466"/>
      <source>Pick aperture angle</source>
      <translation>Trieu l'angle d'obertura</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="509"/>
      <location filename="../../draftguitools/gui_arcs.py" line="551"/>
      <source>Arc by 3 points</source>
      <translation>Arc per 3 punts</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="71"/>
      <location filename="../../draftguitools/gui_lines.py" line="83"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="122"/>
      <location filename="../../draftguitools/gui_rectangles.py" line="69"/>
      <source>Pick first point</source>
      <translation>Trieu el primer punt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="163"/>
      <source>Create Line</source>
      <translation>Crea Línia</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="185"/>
      <source>Create Wire</source>
      <translation>Crea filferro</translation>
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
      <translation>Trieu el següent punt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="330"/>
      <source>Unable to create a Wire from selected objects</source>
      <translation>No es pot crear un filferro a partir dels objectes seleccionats</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="352"/>
      <source>Convert to Wire</source>
      <translation>Converteix en filferro</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="85"/>
      <location filename="../../draftguitools/gui_shapestrings.py" line="98"/>
      <source>Pick ShapeString location point</source>
      <translation>Trieu el punt d'ubicació del ShapeString</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="176"/>
      <location filename="../../draftguitools/gui_shapestrings.py" line="133"/>
      <source>Create ShapeString</source>
      <translation>Crea ShapeString</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="305"/>
      <source>Select a Draft object to edit</source>
      <translation>Seleccioneu un objecte Esbós per editar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="558"/>
      <source>No edit point found for selected object</source>
      <translation>No s`ha trobat cap punt d'edició per l'objecte seleccionat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="811"/>
      <source>Too many objects selected, max number set to:</source>
      <translation>S'han seleccionat massa objectes, el màxim està establert en:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="819"/>
      <source>: this object is not editable</source>
      <translation>: aquest objete no es editable</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="76"/>
      <source>Select an object to join</source>
      <translation>Seleccioneu un objecte per a unir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="99"/>
      <source>Join lines</source>
      <translation>Uneix línies</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="110"/>
      <source>Selection:</source>
      <translation>Selecció:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="64"/>
      <source>Change slope</source>
      <translation>Canvia el pendent</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="94"/>
      <source>Select objects to trim or extend</source>
      <translation>Seleccioneu objectes per a retallar o allargar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="173"/>
      <location filename="../../draftguitools/gui_offset.py" line="143"/>
      <source>Pick distance</source>
      <translation>Trieu la distància</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="222"/>
      <source>Offset angle</source>
      <translation type="unfinished">Offset angle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="483"/>
      <source>Unable to trim these objects, only Draft wires and arcs are supported.</source>
      <translation>No es poden retallar aquests objectes, només es permeten filferros i arcs d'Esbós.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="488"/>
      <source>Unable to trim these objects, too many wires</source>
      <translation>No es poden retallar aquests objectes, massa filferros</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="505"/>
      <source>These objects don't intersect.</source>
      <translation>Aquest objectes no es creuen.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="508"/>
      <source>Too many intersection points.</source>
      <translation>Massa punts d'intersecció.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="66"/>
      <source>Select an object to convert.</source>
      <translation>Seleccioneu un objecte per convertir.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="99"/>
      <source>Convert to Sketch</source>
      <translation>Converteix en Croquis</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="114"/>
      <source>Convert to Draft</source>
      <translation>Converteix a esbós</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="143"/>
      <source>Convert Draft/Sketch</source>
      <translation>Converteix Esbós/Croquis</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="104"/>
      <source>Please select exactly two objects, the base object and the point object, before calling this command.</source>
      <translation>Si us plau, seleccioneu exactament dos objectes, l'objecte base i l'objecte punt, abans d'invocar aquesta ordre.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="122"/>
      <source>Point array</source>
      <translation>Matriu de punts</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="108"/>
      <source>Select an object to edit</source>
      <translation>Selecciona un objecte per a editar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="79"/>
      <source>Select an object to clone</source>
      <translation>Seleccioneu un objecte per a clonar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="212"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="247"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="261"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="294"/>
      <source>Create Dimension</source>
      <translation>Crear dimensió</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="309"/>
      <source>Create Dimension (radial)</source>
      <translation>Crea Cota (radial)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="508"/>
      <source>Edge too short!</source>
      <translation type="unfinished">Edge too short!</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="518"/>
      <source>Edges don't intersect!</source>
      <translation>Les arestes no es creuen!</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="75"/>
      <source>Select an object to stretch</source>
      <translation>Seleccioneu un objecte per estirar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="127"/>
      <source>Pick first point of selection rectangle</source>
      <translation>Trieu el primer punt del rectangle de selecció</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="164"/>
      <source>Pick opposite point of selection rectangle</source>
      <translation>Trieu el punt oposat del rectangle de selecció</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="173"/>
      <source>Pick start point of displacement</source>
      <translation>Seleccioni el punt inicial del desplaçament</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="236"/>
      <source>Pick end point of displacement</source>
      <translation>Seleccioni el punt final del desplaçament</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="448"/>
      <source>Turning one Rectangle into a Wire</source>
      <translation>Transformant un rectangle en un filferro</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="477"/>
      <source>Stretch</source>
      <translation>Estira</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="102"/>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="79"/>
      <source>Please select exactly two objects, the base object and the path object, before calling this command.</source>
      <translation>Si us plau, seleccioneu exactament dos objectes, l'objecte base i l'objecte camí abans d'invocar aquesta ordre.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="101"/>
      <source>Path twisted array</source>
      <translation>Matriu de camí retorçat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="132"/>
      <location filename="../../draftguitools/gui_beziers.py" line="332"/>
      <source>Bézier curve has been closed</source>
      <translation>S’ha tancat la corba Bézier</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="140"/>
      <location filename="../../draftguitools/gui_beziers.py" line="368"/>
      <location filename="../../draftguitools/gui_splines.py" line="131"/>
      <source>Last point has been removed</source>
      <translation>S'ha esborrat l'últim punt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="153"/>
      <location filename="../../draftguitools/gui_splines.py" line="147"/>
      <source>Pick next point, or finish (A) or close (O)</source>
      <translation>Trieu el següent punt, acabeu (A) o tanqueu (O)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="211"/>
      <location filename="../../draftguitools/gui_beziers.py" line="451"/>
      <source>Create BezCurve</source>
      <translation>Crea corba de Bézier</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="376"/>
      <source>Click and drag to define next knot</source>
      <translation>Feu clic i arrossegueu per definir el següent nus</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="382"/>
      <source>Click and drag to define next knot, or finish (A) or close (O)</source>
      <translation>Feu clic i arrossegueu per definir el següent nus, o acabeu (A) o tanqueu (O)</translation>
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
      <translation>Seleccioneu un objecte per a actualitzar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="85"/>
      <source>Downgrade</source>
      <translation>Baixar de categoria</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="143"/>
      <source>Path array</source>
      <translation>Matriu de camí</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="120"/>
      <source>Spline has been closed</source>
      <translation>S'ha tancat l'spline</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="183"/>
      <source>Create B-spline</source>
      <translation>Crea un B-spline</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="125"/>
      <source>Create Plane</source>
      <translation>Crea un pla</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="142"/>
      <source>Create Rectangle</source>
      <translation>Crea un rectangle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="196"/>
      <location filename="../../draftguitools/gui_rectangles.py" line="202"/>
      <source>Pick opposite point</source>
      <translation>Trieu el punt oposat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="76"/>
      <source>Fillet radius</source>
      <translation>Radi del cantell</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="79"/>
      <source>Radius of fillet</source>
      <translation>Radi del cantell</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="107"/>
      <source>Enter radius.</source>
      <translation>Entreu un radi.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="126"/>
      <source>Delete original objects:</source>
      <translation>Esborra els objectes d'origen:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="131"/>
      <source>Chamfer mode:</source>
      <translation>Mode de xamfrà:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="148"/>
      <source>Two elements needed.</source>
      <translation>Calen dos elements.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="155"/>
      <source>Test object</source>
      <translation>Objecte de prova</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="156"/>
      <source>Test object removed</source>
      <translation>S'ha esborrat l'objecte de prova</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="158"/>
      <source>Fillet cannot be created</source>
      <translation>No es pot crear el cantell</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="188"/>
      <source>Create fillet</source>
      <translation>Crea un cantell</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="65"/>
      <source>Add to group</source>
      <translation>Afegeix al grup</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="68"/>
      <source>Ungroup</source>
      <translation>Desagrupa</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="70"/>
      <source>Add new group</source>
      <translation>Afegeix un grup nou</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="159"/>
      <source>Select group</source>
      <translation>Selecciona grup</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="193"/>
      <source>No new selection. You must select non-empty groups or objects inside groups.</source>
      <translation type="unfinished">No new selection. You must select non-empty groups or objects inside groups.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="203"/>
      <source>Autogroup</source>
      <translation>Agrupament automàtic</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="250"/>
      <source>Add new Layer</source>
      <translation>Afegeix una capa nova</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="304"/>
      <source>Add to construction group</source>
      <translation>Afegeix al grup de construcció</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="355"/>
      <source>Add a new group with a given name</source>
      <translation type="unfinished">Add a new group with a given name</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="383"/>
      <source>Add group</source>
      <translation>Afegeix un grup</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="385"/>
      <source>Group name</source>
      <translation>Nom del grup</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="392"/>
      <source>Group</source>
      <translation>Grup</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="92"/>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="569"/>
      <source>This object does not support possible coincident points, please try again.</source>
      <translation>Aquest objecte no suporta possibles punts coincidents, si us plau, torneu a provar-ho.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="164"/>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="608"/>
      <source>Active object must have more than two points/nodes</source>
      <translation>L'objecte actiu ha de tenir més de dos punts/nodes</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="722"/>
      <source>Selection is not a Knot</source>
      <translation>La selecció no és un nus</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="749"/>
      <source>Endpoint of BezCurve can't be smoothed</source>
      <translation>El punt final d'una corba Bézier no es pot suavitzar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="77"/>
      <source>The Drawing Workbench is obsolete since 0.17, consider using the TechDraw Workbench instead.</source>
      <translation>El banc de treball de Dibuix és obsolet des de 0.17, valoreu usar el banc de treball TechDraw en lloc seu.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="81"/>
      <location filename="../../draftguitools/gui_shape2dview.py" line="68"/>
      <source>Select an object to project</source>
      <translation>Seleccioneu un objecte a projectar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="85"/>
      <source>Upgrade</source>
      <translation>Promociona</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="126"/>
      <source>Main toggle snap</source>
      <translation>Commuta l'ajust principal</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="157"/>
      <source>Midpoint snap</source>
      <translation>Ajust al punt central</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="187"/>
      <source>Perpendicular snap</source>
      <translation>Ajust perpendicular</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="217"/>
      <source>Grid snap</source>
      <translation>Ajust de la quadrícula</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="247"/>
      <source>Intersection snap</source>
      <translation>Ajust a la intersecció</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="277"/>
      <source>Parallel snap</source>
      <translation>Ajust paral·lel</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="307"/>
      <source>Endpoint snap</source>
      <translation>Ajust al punt final</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="338"/>
      <source>Angle snap (30 and 45 degrees)</source>
      <translation>Ajust angular (30 i 45 graus)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="368"/>
      <source>Arc center snap</source>
      <translation>Ajust al centre de l'arc</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="398"/>
      <source>Edge extension snap</source>
      <translation>Ajust a l'extensió de l'aresta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="428"/>
      <source>Near snap</source>
      <translation>Ajust proper</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="459"/>
      <source>Orthogonal snap</source>
      <translation>Ajust ortogonal</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="489"/>
      <source>Special point snap</source>
      <translation>Ajust al punt especial</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="520"/>
      <source>Dimension display</source>
      <translation>Mostra la cota</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="553"/>
      <source>Working plane snap</source>
      <translation>Ajust al pla de treball</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="583"/>
      <source>Show snap toolbar</source>
      <translation>Mostra la barra d'eines d'ajustar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="81"/>
      <source>Select an object to move</source>
      <translation>Seleccioneu un objecte per moure</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="103"/>
      <source>Pick start point</source>
      <translation>Trieu el punt d'inici</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="162"/>
      <location filename="../../draftguitools/gui_move.py" line="308"/>
      <source>Pick end point</source>
      <translation>Trieu el punt final</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="210"/>
      <source>Move</source>
      <translation>Mou</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="213"/>
      <location filename="../../draftguitools/gui_rotate.py" line="289"/>
      <source>Some subelements could not be moved.</source>
      <translation>Alguns subelements no s'han pogut moure.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="121"/>
      <location filename="../../draftguitools/gui_ellipses.py" line="138"/>
      <source>Create Ellipse</source>
      <translation>Crea una El·lipse</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="55"/>
      <source>Flip dimension</source>
      <translation>Inverteix l'acotació</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="73"/>
      <source>No active Draft Toolbar.</source>
      <translation>La. barra d'eines Esbós no està activa.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="93"/>
      <source>Construction mode</source>
      <translation>Mode de construcció</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="125"/>
      <source>Continue mode</source>
      <translation>Mode continuar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="159"/>
      <source>Toggle display mode</source>
      <translation>Commuta el mode de visualització</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="68"/>
      <source>Annotation style editor</source>
      <translation>Editor d'estils d'anotació</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="290"/>
      <source>Open styles file</source>
      <translation>Obre el fitxer d'estils</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="292"/>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="314"/>
      <source>JSON file (*.json)</source>
      <translation>Fitxer JSON (*.json)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="312"/>
      <source>Save styles file</source>
      <translation>Desa el fitxer d'estils</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="51"/>
      <source>Heal</source>
      <translation>Reparar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="134"/>
      <location filename="../../draftguitools/gui_points.py" line="147"/>
      <source>Create Point</source>
      <translation>Crea un punt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="224"/>
      <source>Create Polygon (Part)</source>
      <translation>Crea Polígon (Peça)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="243"/>
      <source>Create Polygon</source>
      <translation>Crea Polígon</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="76"/>
      <source>Select an object to offset</source>
      <translation>Trieu un objecte per a desplaçar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="82"/>
      <source>Offset only works on one object at a time.</source>
      <translation>El desplaçament només funcina amb un objecte cada cop.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="92"/>
      <source>Cannot offset this object type</source>
      <translation>No es pot desplaçar aquest tipus d'objecte</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="123"/>
      <source>Offset of Bezier curves is currently not supported</source>
      <translation>El desplaçament de corbes Bézier no és suportat actualment</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="79"/>
      <source>Select an object to rotate</source>
      <translation>Seleccioneu un objecte per girar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="99"/>
      <source>Pick rotation center</source>
      <translation>Trieu el centre de rotació</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="193"/>
      <location filename="../../draftguitools/gui_rotate.py" line="396"/>
      <source>Base angle</source>
      <translation>Angle de base</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="194"/>
      <location filename="../../draftguitools/gui_rotate.py" line="397"/>
      <source>The base angle you wish to start the rotation from</source>
      <translation>L'angle de base d'on voleu iniciar la rotació</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="199"/>
      <location filename="../../draftguitools/gui_rotate.py" line="400"/>
      <source>Pick base angle</source>
      <translation>Trieu angle de base</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="205"/>
      <location filename="../../draftguitools/gui_rotate.py" line="409"/>
      <source>Rotation</source>
      <translation>Rotació</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="206"/>
      <location filename="../../draftguitools/gui_rotate.py" line="410"/>
      <source>The amount of rotation you wish to perform.
The final angle will be the base angle plus this amount.</source>
      <translation>La quantitat de rotació que voleu.
L'angle final serà l'angle de base més aquesta quantitat.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="213"/>
      <location filename="../../draftguitools/gui_rotate.py" line="418"/>
      <source>Pick rotation angle</source>
      <translation>Trieu angle de rotació</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="109"/>
      <source>Create 2D view</source>
      <translation>Crea vista 2D</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="77"/>
      <source>Select an object to array</source>
      <translation>Seleccioneu un objecte per la matriu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="101"/>
      <source>Array</source>
      <translation>Matriu, Array</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="64"/>
      <source>Click anywhere on a line to split it.</source>
      <translation>Cliqueu a aqualsevol punt d'una línia per a dividir-la.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="106"/>
      <source>Split line</source>
      <translation>Separa la línia</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="83"/>
      <source>Task panel:</source>
      <translation>Tauler de tasques:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="187"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="208"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="201"/>
      <source>At least one element must be selected.</source>
      <translation>Cal seleccionar com a mínim un element.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="205"/>
      <source>Number of elements must be at least 1.</source>
      <translation>El nombre d'elements ha de ser com a mínim 1.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="194"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="219"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="212"/>
      <source>Selection is not suitable for array.</source>
      <translation>La selecció no és adequada per a una matriu.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="195"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="327"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="220"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="372"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="213"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="375"/>
      <source>Object:</source>
      <translation>Objecte:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="316"/>
      <source>Interval X reset:</source>
      <translation>Reinicia l'intèrval X:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="325"/>
      <source>Interval Y reset:</source>
      <translation>Reinicia l'intèrval Y:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="334"/>
      <source>Interval Z reset:</source>
      <translation>Reinicia l'intèrval Z:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="296"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="341"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="344"/>
      <source>Fuse:</source>
      <translation>Combina:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="310"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="355"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="358"/>
      <source>Create Link array:</source>
      <translation>Crea matriu d'enllaços:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="376"/>
      <source>Number of X elements:</source>
      <translation>Nombre d'elements X:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="378"/>
      <source>Interval X:</source>
      <translation>Intèrval X:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="381"/>
      <source>Number of Y elements:</source>
      <translation>Nombre d'elements Y:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="383"/>
      <source>Interval Y:</source>
      <translation>Intèrval Y:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="386"/>
      <source>Number of Z elements:</source>
      <translation>Nombre d'elements Z:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="388"/>
      <source>Interval Z:</source>
      <translation>Intèrval Z:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="434"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="481"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="396"/>
      <source>Aborted:</source>
      <translation>Interromput:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="212"/>
      <source>Number of layers must be at least 2.</source>
      <translation>El nombre de capes ha de ser 2 com a mínim.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="224"/>
      <source>Radial distance is zero. Resulting array may not look correct.</source>
      <translation>La distància radial és zero. La matriu resultant pot no semblar correcta.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="226"/>
      <source>Radial distance is negative. It is made positive to proceed.</source>
      <translation>La distància radial és negativa. Es fa positiva per a continuar.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="230"/>
      <source>Tangential distance cannot be zero.</source>
      <translation>La distància tangencial no pot ser zero.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="233"/>
      <source>Tangential distance is negative. It is made positive to proceed.</source>
      <translation>La distància tangencial és negativa. Es torna positiva per continuar.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="286"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="331"/>
      <source>Center reset:</source>
      <translation>Reinicia el centre:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="373"/>
      <source>Radial distance:</source>
      <translation>Distància radial:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="374"/>
      <source>Tangential distance:</source>
      <translation>Distància tangencial:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="375"/>
      <source>Number of circular layers:</source>
      <translation>Nombre de capes circulars:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="376"/>
      <source>Symmetry parameter:</source>
      <translation>Paràmetre de simetria:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="331"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="378"/>
      <source>Center of rotation:</source>
      <translation>Centre de rotació:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="199"/>
      <source>Number of elements must be at least 2.</source>
      <translation>El nombre d'elements ha de ser 2 com a mínim.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="203"/>
      <source>The angle is above 360 degrees. It is set to this value to proceed.</source>
      <translation>L'angle és major de 360 graus. S'estableix a aquest valor per continuar.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="206"/>
      <source>The angle is below -360 degrees. It is set to this value to proceed.</source>
      <translation>L'angle és menor a -360 graus. S'estableix a aquest valor per continuar.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="328"/>
      <source>Number of elements:</source>
      <translation>Nombre d'elements:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="329"/>
      <source>Polar angle:</source>
      <translation>Angle polar:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="56"/>
      <source>ShapeString</source>
      <translation>Forma d'Etiqueta</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="70"/>
      <source>Default</source>
      <translation>Per defecte</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="361"/>
      <source>Activate this layer</source>
      <translation>Activa aquesta capa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="367"/>
      <source>Select layer contents</source>
      <translation>Seleccioneu el contingut de la capa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="405"/>
      <location filename="../../draftviewproviders/view_layer.py" line="421"/>
      <source>Merge layer duplicates</source>
      <translation>Fusiona duplicats de capa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="410"/>
      <location filename="../../draftviewproviders/view_layer.py" line="469"/>
      <source>Add new layer</source>
      <translation>Afegeix una capa</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="454"/>
      <source>Relabeling layer:</source>
      <translation type="unfinished">Relabeling layer:</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="458"/>
      <source>Merging layer:</source>
      <translation type="unfinished">Merging layer:</translation>
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
      <translation>S'ha produït un error OCA: no s'ha pogut determinar la codificació de caràcters</translation>
    </message>
    <message>
      <location filename="../../importOCA.py" line="445"/>
      <source>OCA: found no data to export</source>
      <translation>OCA: no s'ha trobat cap dada per a exportar</translation>
    </message>
    <message>
      <location filename="../../importOCA.py" line="490"/>
      <source>successfully exported</source>
      <translation>s'ha exportat amb èxit</translation>
    </message>
  </context>
</TS>
