<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="tr" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../draftobjects/wire.py" line="49"/>
      <source>The vertices of the wire</source>
      <translation>Telin köşeleri</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="53"/>
      <source>If the wire is closed or not</source>
      <translation>Tel kapalı veya değil ise</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="57"/>
      <source>The base object is the wire, it's formed from 2 objects</source>
      <translation>Temel nesne teldir, 2 nesneden oluşur</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="61"/>
      <source>The tool object is the wire, it's formed from 2 objects</source>
      <translation>Araç nesnesi teldir, 2 nesneden oluşur</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="65"/>
      <source>The start point of this line</source>
      <translation>Bu çizginin başlangıç noktası</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="69"/>
      <source>The end point of this line</source>
      <translation>Bu çizginin bitiş noktası</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="73"/>
      <source>The length of this line</source>
      <translation>Bu çizginin uzunluğu</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="52"/>
      <location filename="../../draftobjects/polygon.py" line="60"/>
      <location filename="../../draftobjects/wire.py" line="77"/>
      <source>Radius to use to fillet the corners</source>
      <translation>Köşeleri yuvarlamak için kullanılacak yarıçap</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="55"/>
      <location filename="../../draftobjects/polygon.py" line="64"/>
      <location filename="../../draftobjects/wire.py" line="81"/>
      <source>Size of the chamfer to give to the corners</source>
      <translation>Köşelere verilecek pahın ölçüsü</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="85"/>
      <source>Create a face if this object is closed</source>
      <translation>Bu nesne kapalı ise yüzey oluştur</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="89"/>
      <source>The number of subdivisions of each edge</source>
      <translation>Her kenarın alt bölümlerinin sayısı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="67"/>
      <location filename="../../draftobjects/circle.py" line="62"/>
      <location filename="../../draftobjects/polygon.py" line="72"/>
      <location filename="../../draftobjects/bspline.py" line="57"/>
      <location filename="../../draftobjects/bezcurve.py" line="70"/>
      <location filename="../../draftobjects/wire.py" line="93"/>
      <source>The area of this object</source>
      <translation>Bu objenin alanı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="65"/>
      <source>The position of the tip of the leader line.
This point can be decorated with an arrow or another symbol.</source>
      <translation>Kılavuz çizgisinin ucunun konumu.
Bu nokta bir ok veya başka bir sembolle süslenebilir.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="79"/>
      <source>Object, and optionally subelement, whose properties will be displayed
as 'Text', depending on 'Label Type'.

'Target' won't be used if 'Label Type' is set to 'Custom'.</source>
      <translation>Nesne ve isteğe bağlı olarak, özellikleri 'Etiket türüne' bağlı olarak 
'metin'olarak görüntülenecek olan alt öğe.

'Etiket türü '' özel 'olarak ayarlanırsa'hedef' kullanılmaz.</translation>
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
      <translation>Lider çizgisini tanımlayan noktaların listesi; normalde üç noktadan oluşan bir liste.

İlk nokta metnin konumu yani 'Yerleşim' olmalıdır,
ve son nokta, çizginin ucu yani 'Hedef Nokta' olmalıdır.
Orta nokta, seçilene bağlı olarak otomatik olarak hesaplanır.
'Düz Yön' ve 'Düz Mesafe' değeri ve işareti.

'Düz Yön' 'Özel' olarak ayarlanmışsa, 'Noktalar' özelliği
keyfi noktaların bir listesi olarak ayarlanabilir.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="124"/>
      <source>The direction of the straight segment of the leader line.

If 'Custom' is chosen, the points of the leader can be specified by
assigning a custom list to the 'Points' attribute.</source>
      <translation>Lider çizginin düz parçasının yönü.

'Özel' seçilirse, liderin noktaları şu şekilde belirlenebilir:
'Nokta' özelliğine özel bir liste atama.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="142"/>
      <source>The length of the straight segment of the leader line.

This is an oriented distance; if it is negative, the line will be drawn
to the left or below the 'Text', otherwise to the right or above it,
depending on the value of 'Straight Direction'.</source>
      <translation>Lider çizgisinin düz parçasının uzunluğu.

Bu, yönlendirilmiş bir mesafedir; negatifse, çizgi çizilecektir
'Metin'in solunda veya altında, aksi takdirde sağında veya üstünde,
'Düz Yön' değerine bağlı olarak.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="156"/>
      <source>The placement of the 'Text' element in 3D space</source>
      <translation>'Metin' öğesinin 3B alana yerleştirilmesi</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="166"/>
      <source>The text to display when 'Label Type' is set to 'Custom'</source>
      <translation>"Etiket Türü" "Özel" olarak ayarlandığında görüntülenecek metin</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="181"/>
      <source>The text displayed by this label.

This property is read-only, as the final text depends on 'Label Type',
and the object defined in 'Target'.
The 'Custom Text' is displayed only if 'Label Type' is set to 'Custom'.</source>
      <translation>Bu etikette görüntülenen metin.

Bu özellik salt okunur, çünkü son metin 'Etiket Türüne' ve 'hedef'
içinde tanımlanan nesneye bağlıdır.
'Özel metin' yalnızca 'Etiket türü' 'özel'olarak ayarlanmışsa görüntülenir.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="216"/>
      <source>The type of information displayed by this label.

If 'Custom' is chosen, the contents of 'Custom Text' will be used.
For other types, the string will be calculated automatically from the object defined in 'Target'.
'Tag' and 'Material' only work for objects that have these properties, like Arch objects.

For 'Position', 'Length', and 'Area' these properties will be extracted from the main object in 'Target',
or from the subelement 'VertexN', 'EdgeN', or 'FaceN', respectively, if it is specified.</source>
      <translation>Bu etiket tarafından görüntülenen bilgi türü.

'Özel' seçilirse, 'Özel Metin' içeriği kullanılacaktır.
Diğer türler için, dizi, 'Hedef'te tanımlanan nesneden otomatik olarak hesaplanacaktır.
"Etiket" ve "Malzeme", Arch nesneleri gibi yalnızca bu özelliklere sahip nesneler için çalışır.

'Konum', 'Uzunluk' ve 'Alan' için bu özellikler 'Hedef' içindeki ana nesneden çıkarılacaktır,
veya belirtilmişse, sırasıyla 'VertexN', 'EdgeN' veya 'FaceN' alt öğesinden.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="46"/>
      <source>The base object used by this object</source>
      <translation>Bu nesnenin kullandığı temel alınan nesne</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="49"/>
      <source>The PAT file used by this object</source>
      <translation>Bu nesnenin kullandığı PAT dosyası</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="52"/>
      <source>The pattern name used by this object</source>
      <translation>Bu nesnenin kullandığı desen adı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="55"/>
      <source>The pattern scale used by this object</source>
      <translation>Bu nesnenin kullandığı desen ölçeği</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="58"/>
      <source>The pattern rotation used by this object</source>
      <translation>Bu nesnenin kullandığı desen dönüş açısı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="61"/>
      <source>If set to False, hatch is applied as is to the faces, without translation (this might give wrong results for non-XY faces)</source>
      <translation>If set to False, hatch is applied as is to the faces, without translation (this might give wrong results for non-XY faces)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="66"/>
      <source>The linked object</source>
      <translation>Bağlantı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="73"/>
      <source>Projection direction</source>
      <translation>Projeksiyon yön</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="80"/>
      <source>The width of the lines inside this object</source>
      <translation>Bu nesnenin içindeki çizgilerin kalınlığı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="88"/>
      <source>The size of the texts inside this object</source>
      <translation>Bu nesnenin içindeki metinlerin boyutu</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="96"/>
      <source>The spacing between lines of text</source>
      <translation>Metin satırları arasındaki boşluk</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="103"/>
      <source>The color of the projected objects</source>
      <translation>Yansıtılan nesnelerin rengi</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="110"/>
      <source>Shape Fill Style</source>
      <translation>Şekil Dolgu Biçimi</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="118"/>
      <source>Line Style</source>
      <translation>Çizgi Stili</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="127"/>
      <source>If checked, source objects are displayed regardless of being visible in the 3D model</source>
      <translation>İşaretlenirse, görünür olmasına bakılmaksızın kaynak nesneler 3D modelde görüntülenir</translation>
    </message>
    <message>
      <location filename="../../draftobjects/circle.py" line="45"/>
      <source>Start angle of the arc</source>
      <translation>Yayın başlangıç açısı</translation>
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
      <translation>Çemberin yarıçapı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="58"/>
      <location filename="../../draftobjects/circle.py" line="58"/>
      <location filename="../../draftobjects/polygon.py" line="68"/>
      <location filename="../../draftobjects/ellipse.py" line="58"/>
      <source>Create a face</source>
      <translation>Bir yüzey oluştur</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="46"/>
      <source>Text string</source>
      <translation>Metin dizesi</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="49"/>
      <source>Font file name</source>
      <translation>Yazı tipi dosyası adı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="52"/>
      <source>Height of text</source>
      <translation>Satır yüksekliği</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="55"/>
      <source>Inter-character spacing</source>
      <translation>Karakterler arası boşluk</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="58"/>
      <source>Fill letters with faces</source>
      <translation>Harfleri yüzeyle doldur</translation>
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
      <translation>Kopyaların dağıtılacağı nesne. "Kenarlar" içermelidir.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="100"/>
      <source>Number of copies to create.</source>
      <translation>Oluşturulacak kopyaların sayısı.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="107"/>
      <source>Rotation factor of the twisted array.</source>
      <translation>Bükülmüş dizi dönme etkeni.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="320"/>
      <location filename="../../draftobjects/pathtwistedarray.py" line="114"/>
      <location filename="../../draftobjects/pointarray.py" line="112"/>
      <location filename="../../draftobjects/patharray.py" line="208"/>
      <source>Show the individual array elements (only for Link arrays)</source>
      <translation>Bireysel dizi öğelerini gösterin (yalnızca bağlantı dizileri için)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="83"/>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="82"/>
      <source>General scaling factor that affects the annotation consistently
because it scales the text, and the line decorations, if any,
in the same proportion.</source>
      <translation>Açıklamayı tutarlı bir şekilde etkileyen genel ölçekleme faktörü
metni ve varsa çizgi süslemelerini ölçeklendirdiği için,
aynı oranda olur.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="93"/>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="100"/>
      <source>Annotation style to apply to this object.
When using a saved style some of the view properties will become read-only;
they will only be editable by changing the style through the 'Annotation style editor' tool.</source>
      <translation>Bu nesneye uygulanacak açıklama stili.
Kaydedilmiş bir stili kullanırken bazı görünüm özellikleri salt okunur hale gelecektir;
bunlar yalnızca 'Ek açıklama stili düzenleyici' aracıyla stil değiştirilerek düzenlenebilir.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draftlink.py" line="99"/>
      <source>Force sync pattern placements even when array elements are expanded</source>
      <translation>Force sync pattern placements even when array elements are expanded</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draftlink.py" line="112"/>
      <source>Show the individual array elements</source>
      <translation>Bağımsız dizi elemanlarını göster</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="46"/>
      <source>The objects included in this clone</source>
      <translation>Bu klon da bulunan nesneler</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="51"/>
      <source>The scale factor of this clone</source>
      <translation>Bu klonun ölçek faktörü</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="57"/>
      <source>If Clones includes several objects,
set True for fusion or False for compound</source>
      <translation>Klonlar birkaç nesne içeriyorsa, 
füzyon için Doğru veya bileşik için Yanlış olarak ayarlayın</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="46"/>
      <source>The points of the B-spline</source>
      <translation>B-spline'ın noktaları</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="50"/>
      <source>If the B-spline is closed or not</source>
      <translation>B-spline kapalı veya değilse</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="54"/>
      <source>Create a face if this spline is closed</source>
      <translation>Bu spline kapalıysa yüzey oluştur</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="67"/>
      <source>Parameterization factor</source>
      <translation>Parametrelendirme etkeni</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="57"/>
      <source>The base object this 2D view must represent</source>
      <translation>Temel nesne bu 2B görünümü temsil etmelidir</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="62"/>
      <source>The projection vector of this object</source>
      <translation>Bu nesnenin izdüşüm vektörü</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="68"/>
      <source>The way the viewed object must be projected</source>
      <translation>Görüntülenen nesnenin yansıtılma şekli</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="75"/>
      <source>The indices of the faces to be projected in Individual Faces mode</source>
      <translation>Bireysel yüz modunda projelendirilen yüzler endeksleri</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="80"/>
      <source>Show hidden lines</source>
      <translation>Gizli çizgileri göster</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="86"/>
      <source>Fuse wall and structure objects of same type and material</source>
      <translation>Sigorta duvarı ve yapı nesneleri ile aynı tip ve malzemeden</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="91"/>
      <source>Tessellate Ellipses and B-splines into line segments</source>
      <translation>Desenli Elipsler ve B-spline'ları çizgi parçalarına ayırır</translation>
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
      <translation>Eğer bu Doğru ise bu nesne sadece görünür nesneler içerecek</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="117"/>
      <source>A list of exclusion points. Any edge touching any of those points will not be drawn.</source>
      <translation>Hariç tutulan noktalar listesi. Bu noktaların herhangi birine değen hiçbir kenar çizilmeyecek.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="122"/>
      <source>If this is True, only solid geometry is handled. This overrides the base object's Only Solids property</source>
      <translation>Eğer bu Doğru olarak belirtilmişse, sadece katı geometri işlenir. Bu durum, temel nesnenin Sadece Katılar özelliği üzerine yazılır</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="127"/>
      <source>If this is True, the contents are clipped to the borders of the section plane, if applicable. This overrides the base object's Clip property</source>
      <translation>Eğer bu Doğru olarak belirtilmişse, uygulanabilmesi durumunda, içerik kesit düzlemi sınırlarına kırpılır. Bu durum, temel nesnenin Kırpık özelliği üzerine yazılır</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="132"/>
      <source>This object will be recomputed only if this is True.</source>
      <translation>Bu nesne, sadece bu değişken Doğru olarak belirtilirse yeniden hesaplanacak.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="45"/>
      <source>X Location</source>
      <translation>X Konumu</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="48"/>
      <source>Y Location</source>
      <translation>Y Konumu</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="51"/>
      <source>Z Location</source>
      <translation>Z Konumu</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="46"/>
      <source>Length of the rectangle</source>
      <translation>Dikdörtgenin genişliği</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="49"/>
      <source>Height of the rectangle</source>
      <translation>Dikdörtgenin yüksekliği</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="61"/>
      <source>Horizontal subdivisions of this rectangle</source>
      <translation>Bu dikdörtgenin yatay alt bölümleri</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="64"/>
      <source>Vertical subdivisions of this rectangle</source>
      <translation>Bu dikdörtgenin dikey alt bölümleri</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="43"/>
      <source>Linked faces</source>
      <translation>Bağlantılı yüzeyler</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="46"/>
      <source>Specifies if splitter lines must be removed</source>
      <translation>Bölme çizgilerinin kaldırılması gerekip gerekmeyeceğini belirtir</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="49"/>
      <source>An optional extrusion value to be applied to all faces</source>
      <translation>Tüm yüzlere uygulanacak isteğe bağlı bir ekstrüzyon değeri</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="52"/>
      <source>An optional offset value to be applied to all faces</source>
      <translation>Tüm yüzlere uygulanacak isteğe bağlı bir ofset değeri</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="55"/>
      <source>This specifies if the shapes sew</source>
      <translation>Bu özellik şekillerin dilieceğini veya dikilmeyeceğini belirler</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="58"/>
      <source>The area of the faces of this Facebinder</source>
      <translation>Bu Facebinder'in yüzlerinin alanı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="48"/>
      <source>Number of faces</source>
      <translation>Yüzeylerin sayısı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="52"/>
      <source>Radius of the control circle</source>
      <translation>Kontrol çemberinin yarıçapı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="56"/>
      <source>How the polygon must be drawn from the control circle</source>
      <translation>Çokgen kontrol çemberinden nasıl çizilmeli</translation>
    </message>
    <message>
      <location filename="../../draftobjects/block.py" line="43"/>
      <source>The components of this block</source>
      <translation>Bileşenler bu bloğa aittir</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="47"/>
      <source>The start point of this line.</source>
      <translation>Bu çizginin başlangıç noktası.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="55"/>
      <source>The end point of this line.</source>
      <translation>Bu çizginin bitiş noktası.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="63"/>
      <source>The length of this line.</source>
      <translation>Bu çizginin uzunluğu.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="71"/>
      <source>Radius to use to fillet the corner.</source>
      <translation>Köşeleri yuvarlamak için kullanılacak yarıçap.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wpproxy.py" line="42"/>
      <source>The placement of this object</source>
      <translation>Bu nesnenin yerleşimi</translation>
    </message>
    <message>
      <location filename="../../draftobjects/layer.py" line="59"/>
      <source>The objects that are part of this layer</source>
      <translation>Bu katmanın bir parçası olan nesneler</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="137"/>
      <source>The normal direction of the text of the dimension</source>
      <translation>Ölçülendirme yazısının normal yönü</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="150"/>
      <source>The object measured by this dimension object</source>
      <translation>Obje ölçümü boyutlandırma nesnesi tarafından yapıldı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="167"/>
      <source>The object, and specific subelements of it,
that this dimension object is measuring.

There are various possibilities:
- An object, and one of its edges.
- An object, and two of its vertices.
- An arc object, and its edge.</source>
      <translation>Nesne ve onun belirli alt öğeleri,
bu boyut nesnesinin ölçüyor olması.

Çeşitli olasılıklar vardır:
- Bir nesne ve kenarlarından biri.
- Bir nesne ve iki köşesi.
- Bir çemberYayı nesnesi ve kenarı.</translation>
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
      <translation>Ölçü çizgisinin veya bunun bir ekstrapolasyonunun geçeceği bir nokta.

- Doğrusal Ölçüler için bu özellik, Ölçü çizgisinin ölçülen nesneye ne kadar 
yakın olduğunu kontrol eder.
- Radyal Ölçüler için bu, ölçülen yarıçapı veya çapı görüntüleyen ölçü 
çizgisinin yönünü kontrol eder.
- Açısal Ölçüler için bu, ölçülen açıyı görüntüleyen ölçü yayının yarıçapını 
kontrol eder.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="233"/>
      <source>Starting point of the dimension line.

If it is a radius dimension it will be the center of the arc.
If it is a diameter dimension it will be a point that lies on the arc.</source>
      <translation>Ölçü çizgisinin başlangıç noktası.

Bir yarıçap ölçüsüyse, yayın merkezi olacaktır.
Çap ölçüsü ise, yay üzerinde uzanan bir nokta olacaktır.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="247"/>
      <source>Ending point of the dimension line.

If it is a radius or diameter dimension
it will be a point that lies on the arc.</source>
      <translation>Ölçü çizgisinin bitiş noktası.

Bir yarıçap veya çap boyutu ise,
yay üzerinde uzanan bir nokta olacaktır.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="259"/>
      <source>The direction of the dimension line.
If this remains '(0,0,0)', the direction will be calculated automatically.</source>
      <translation>Ölçü çizgisinin yönü.
Bu '(0,0,0)' olarak kalırsa, yön otomatik olarak hesaplanacaktır.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="276"/>
      <source>The value of the measurement.

This property is read-only because the value is calculated
from the 'Start' and 'End' properties.

If the 'Linked Geometry' is an arc or circle, this 'Distance'
is the radius or diameter, depending on the 'Diameter' property.</source>
      <translation>Ölçümün değeri.

Bu özellik salt okunurdur çünkü değer "Başlangıç" 
ve "Bitiş" özelliklerinden hesaplanır.

'Bağlantılı Geometri' bir yay veya daire ise, bu 'Uzaklık', 
'Çap' özelliğine bağlı olarak yarıçap veya çaptır.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="287"/>
      <source>When measuring circular arcs, it determines whether to display
the radius or the diameter value</source>
      <translation>Dairesel yayları ölçerken, yarıçap veya çap değeri 
gösterilip gösterilmeyeceğini belirler</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="507"/>
      <source>Starting angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
      <translation>Ölçü çizgisinin başlangıç açısı (dairesel yay).
Yay, saat yönünün tersine çizilir.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="518"/>
      <source>Ending angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
      <translation>Ölçü çizgisinin bitiş açısı (dairesel yay).
Yay, saat yönünün tersine çizilir.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="533"/>
      <source>The center point of the dimension line, which is a circular arc.

This is normally the point where two line segments, or their extensions
intersect, resulting in the measured 'Angle' between them.</source>
      <translation>Dairesel bir yaya ait ölçü çizgisinin merkez noktası.

Bu normalde iki çizgi parçasının veya uzantılarının kesiştiği noktadır 
ve aralarında ölçülen 'Açı' ile sonuçlanır.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="547"/>
      <source>The value of the measurement.

This property is read-only because the value is calculated from
the 'First Angle' and 'Last Angle' properties.</source>
      <translation>Ölçüm değerleri.

Bu özellik salt okunurdur çünkü değer şu kaynaktan hesaplanır:
'İlk Açı' ve 'Son Açı' özellikleri.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="45"/>
      <source>Start angle of the elliptical arc</source>
      <translation>Eliptik yayın başlangıç açısı</translation>
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
      <translation>Elipsin küçük yarıçapı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="55"/>
      <source>Major radius of the ellipse</source>
      <translation>Elipsin büyük yarıçapı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="61"/>
      <source>Area of this object</source>
      <translation>Bu nesnenin alanı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/text.py" line="54"/>
      <source>The placement of the base point of the first line</source>
      <translation>İlk çizginin taban noktasının yerleştirilmesi</translation>
    </message>
    <message>
      <location filename="../../draftobjects/text.py" line="66"/>
      <source>The text displayed by this object.
It is a list of strings; each element in the list will be displayed in its own line.</source>
      <translation>Bu nesne tarafından görüntülenen metin.
Bu, dizelerin (strings) bir listesidir; listedeki her öğe kendi satırında görüntülenecektir.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="82"/>
      <location filename="../../draftobjects/patharray.py" line="169"/>
      <source>The base object that will be duplicated</source>
      <translation>Ana obje çoğaltılmış olacaktır</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="192"/>
      <source>List of connected edges in the 'Path Object'.
If these are present, the copies will be created along these subelements only.
Leave this property empty to create copies along the entire 'Path Object'.</source>
      <translation>'Yol Nesnesi'ndeki bağlantılı kenarların listesi.
Bunlar mevcutsa, kopyalar yalnızca bu alt öğeler boyunca oluşturulacaktır.
Tüm 'Yol Nesnesi' boyunca kopyalar oluşturmak için bu özelliği boş bırakın.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="200"/>
      <source>Number of copies to create</source>
      <translation>Oluşturulacak kopyaların sayısı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="219"/>
      <source>Additional translation that will be applied to each copy.
This is useful to adjust for the difference between shape centre and shape reference point.</source>
      <translation>Her kopyaya uygulanacak ek çeviri.
Bu, şekil merkezi ve şekil referans noktası arasındaki farkı ayarlamak için kullanışlıdır.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="227"/>
      <source>Alignment vector for 'Tangent' mode</source>
      <translation>'Teğet' modu için hizalama vektörü</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="235"/>
      <source>Force use of 'Vertical Vector' as local Z direction when using 'Original' or 'Tangent' alignment mode</source>
      <translation>"Orijinal" veya "Teğet" hizalama modunu kullanırken yerel Z yönü olarak "Dikey Vektör" kullanımını zorla</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="243"/>
      <source>Direction of the local Z axis when 'Force Vertical' is true</source>
      <translation>'Dikeyi Zorla' doğru olduğunda yerel Z ekseninin yönü</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="251"/>
      <source>Method to orient the copies along the path.
- Original: X is curve tangent, Y is normal, and Z is the cross product.
- Frenet: aligns the object following the local coordinate system along the path.
- Tangent: similar to 'Original' but the local X axis is pre-aligned to 'Tangent Vector'.

To get better results with 'Original' or 'Tangent' you may have to set 'Force Vertical' to true.</source>
      <translation>Kopyaları yol boyunca yönlendirme yöntemi.
- Orijinal: X eğri tanjantıdır, Y normaldir ve Z çapraz çarpımdır.
- Frenet: Yol boyunca yerel koordinat sistemini takip eden nesneyi hizalar.
- Teğet: "Orijinal" e benzer, ancak yerel X ekseni "Teğet Vektör" e önceden hizalanmıştır.

"Orijinal" veya "Teğet" ile daha iyi sonuçlar elde etmek için "Dikey Zorla" yı doğru olarak ayarlamanız gerekebilir.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="262"/>
      <source>Orient the copies along the path depending on the 'Align Mode'.
Otherwise the copies will have the same orientation as the original Base object.</source>
      <translation>'Hizalama Modu'na bağlı olarak kopyaları yol boyunca yönlendirin.
Aksi takdirde kopyalar orijinal ana nesnesiyle aynı yönlendirmeye sahip olacaktır.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="46"/>
      <source>The points of the Bezier curve</source>
      <translation>Bezier eğrisinin noktaları</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="50"/>
      <source>The degree of the Bezier function</source>
      <translation>Bezier fonksiyonunun derecesi</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="54"/>
      <source>Continuity</source>
      <translation>Süreklilik</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="58"/>
      <source>If the Bezier curve should be closed or not</source>
      <translation>Bezier eğrisinin kapatılması gerekip gerekmediği</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="62"/>
      <source>Create a face if this curve is closed</source>
      <translation>Bu eğri kapalıysa bir yüzey oluştur</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="66"/>
      <source>The length of this object</source>
      <translation>Bu nesnenin uzunluğu</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="98"/>
      <source>The type of array to create.
- Ortho: places the copies in the direction of the global X, Y, Z axes.
- Polar: places the copies along a circular arc, up to a specified angle, and with certain orientation defined by a center and an axis.
- Circular: places the copies in concentric circular layers around the base object.</source>
      <translation>Oluşturulacak dizilim türü.
-Ortho: Kopyaları genel kordinat sisteminde x, y ve z aksları yönlerinde oluşturur.
-Polar: Kopyaları dairesel veya yay üzerinde oluşturur. özelikle bir açı veya bir merkez noktası aksı etrafında kopyalar.
-Circular: Kopyaları temel objenin etrafındaki dairesel ilişkilendirilmiş katman etrafında oluşturur.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="109"/>
      <source>Specifies if the copies should be fused together if they touch each other (slower)</source>
      <translation>Birbirlerine temas ettiklerinde kopyaların birbirine kaynaştırılıp birleştirileceğini belirtir (çok yavaş)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="122"/>
      <source>Number of copies in X direction</source>
      <translation>X doğrultusundaki kopya sayısı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="131"/>
      <source>Number of copies in Y direction</source>
      <translation>Y doğrultusundaki kopya sayısı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="140"/>
      <source>Number of copies in Z direction</source>
      <translation>Z doğrultusundaki kopya sayısı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="150"/>
      <source>Distance and orientation of intervals in X direction</source>
      <translation>X doğrultusundaki aralıkların oryantasyonu ve uzunlukları</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="160"/>
      <source>Distance and orientation of intervals in Y direction</source>
      <translation>Y doğrultusundaki aralıkların oryantasyonu ve uzunlukları</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="170"/>
      <source>Distance and orientation of intervals in Z direction</source>
      <translation>Z doğrultusundaki aralıkların oryantasyonu ve uzunlukları</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="185"/>
      <source>The axis direction around which the elements in a polar or a circular array will be created</source>
      <translation>Kutupsal veya dairesel bir dizideki öğelerin oluşturulacağı eksen yönü</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="196"/>
      <source>Center point for polar and circular arrays.
The 'Axis' passes through this point.</source>
      <translation>Kutupsal ve dairesel diziler için merkez nokta.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="215"/>
      <source>The axis object that overrides the value of 'Axis' and 'Center', for example, a datum line.
Its placement, position and rotation, will be used when creating polar and circular arrays.
Leave this property empty to be able to set 'Axis' and 'Center' manually.</source>
      <translation>Örneğin bir mevki çizgisi gibi 'Eksen' ve 'Merkez' değerini geçersiz kılan eksen nesnesi.
Kutupsal ve dairesel diziler oluşturulurken yerleşimi, konumu ve dönüşü kullanılacaktır.
'Eksen' ve 'Merkez'i manuel olarak ayarlayabilmek için bu özelliği boş bırakın.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="228"/>
      <source>Number of copies in the polar direction</source>
      <translation>Kutup yönündeki kopya sayısı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="238"/>
      <source>Distance and orientation of intervals in 'Axis' direction</source>
      <translation>Aralıkların 'Eksen' yönündeki uzaklığı ve yönü</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="247"/>
      <source>Angle to cover with copies</source>
      <translation>Kopyaları örtme açısı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="260"/>
      <source>Distance between circular layers</source>
      <translation>Dairesel katmanlar arasındaki uzaklık</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="270"/>
      <source>Distance between copies in the same circular layer</source>
      <translation>Aynı dairesel katman kopyaları arasındaki uzaklık</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="280"/>
      <source>Number of circular layers. The 'Base' object counts as one layer.</source>
      <translation>Dairesel katman adedi. temel objei tek katman olarak sayılır.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="291"/>
      <source>A parameter that determines how many symmetry planes the circular array will have.</source>
      <translation>Dairesel dizilimin kaç tane simetri düzlemine sahip olacağını belirleyen değişken.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="309"/>
      <source>Total number of elements in the array.
This property is read-only, as the number depends on the parameters of the array.</source>
      <translation>Dizideki toplam öğe sayısı.
Bu özellik salt okunurdur; çünkü sayılar, dizinin parametrelerine bağlıdır.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="79"/>
      <source>Base object that will be duplicated</source>
      <translation>Çoğaltılacak temel nesne</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="87"/>
      <source>Object containing points used to distribute the base object, for example, a sketch or a Part compound.
The sketch or compound must contain at least one explicit point or vertex object.</source>
      <translation>Temel nesneyi dağıtmak için kullanılan noktaları içeren nesne, örneğin çizim veya Parça bileşiği.
Çizim veya bileşik en az bir açık nokta veya köşe nesnesi içermelidir.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="95"/>
      <source>Total number of elements in the array.
This property is read-only, as the number depends on the points contained within 'Point Object'.</source>
      <translation>Dizideki toplam öğe sayısı.
Bu özellik salt okunurdur, çünkü sayı 'Nokta Nesnesi'nde bulunan noktalara bağlıdır.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="104"/>
      <location filename="../../draftobjects/pointarray.py" line="140"/>
      <source>Additional placement, shift and rotation, that will be applied to each copy</source>
      <translation>Her kopyaya uygulanacak ek yerleştirme, kaydırma ve döndürme</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="60"/>
      <location filename="../../draftviewproviders/view_label.py" line="74"/>
      <source>The size of the text</source>
      <translation>Metnin boyutu</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="69"/>
      <location filename="../../draftviewproviders/view_label.py" line="83"/>
      <source>The font of the text</source>
      <translation>Metnin yazı tipi</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="78"/>
      <location filename="../../draftviewproviders/view_label.py" line="92"/>
      <location filename="../../draftviewproviders/view_label.py" line="119"/>
      <source>The vertical alignment of the text</source>
      <translation>Metnin dikey hizalaması</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="87"/>
      <location filename="../../draftviewproviders/view_label.py" line="102"/>
      <source>Text color</source>
      <translation>Metin rengi</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="95"/>
      <location filename="../../draftviewproviders/view_label.py" line="128"/>
      <source>Line spacing (relative to font size)</source>
      <translation>Satır boşluğu (yazı tipi boyutuna göre)</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="111"/>
      <source>The maximum number of characters on each line of the text box</source>
      <translation>Metin kutusunun her satırındaki maksimum karakter sayısı</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="139"/>
      <source>The size of the arrow</source>
      <translation>Ok İşaretinin boyutu</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="148"/>
      <source>The type of arrow of this label</source>
      <translation>Bu etiketin ok işareti türü</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="159"/>
      <source>The type of frame around the text of this object</source>
      <translation>Bu nesnenin metin etrafında çerçeve türü</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="168"/>
      <source>Display a leader line or not</source>
      <translation>Kılavuz hattını göster ya da gösterme</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="115"/>
      <location filename="../../draftviewproviders/view_label.py" line="177"/>
      <source>Line width</source>
      <translation>Çizgi Kalınlığı</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="122"/>
      <location filename="../../draftviewproviders/view_label.py" line="186"/>
      <source>Line color</source>
      <translation>Çizgi rengi</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="137"/>
      <source>Font name</source>
      <translation>Yazı tipi ismi</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="146"/>
      <source>Font size</source>
      <translation>Yazı Boyutu</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="155"/>
      <source>Spacing between text and dimension line</source>
      <translation>Metin ve ölçü çizgisi arasındaki boşluk</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="164"/>
      <source>Rotate the dimension text 180 degrees</source>
      <translation>Ölçüm yazısını 180 derece döndürür</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="174"/>
      <source>Text Position.
Leave '(0,0,0)' for automatic position</source>
      <translation>Metin Konumu.
Otomatik konum için '(0,0,0)' bırakın</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="185"/>
      <source>Text override.
Write '$dim' so that it is replaced by the dimension length.</source>
      <translation>Metni geçersiz kıl.
Ölçü uzunluğu ile değiştirilecek şekilde '$dim' yazın.</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="196"/>
      <source>The number of decimals to show</source>
      <translation>Gösterilecek ondalık basamaklarının sayısı</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="205"/>
      <source>Show the unit suffix</source>
      <translation>Birim son eki</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="216"/>
      <source>A unit to express the measurement.
Leave blank for system default.
Use 'arch' to force US arch notation</source>
      <translation>Ölçümlendirmeyi gösterecek birim.
Sistem varsayılanı için boş bırakın.
BM mimari gösterimi için 'mimari' yi kullanın</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="230"/>
      <source>Arrow size</source>
      <translation>Ok boyu</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="239"/>
      <source>Arrow type</source>
      <translation>Ok tipi</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="249"/>
      <source>Rotate the dimension arrows 180 degrees</source>
      <translation>Ölçüm oklarını 180 derece döndürür</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="260"/>
      <source>The distance the dimension line is extended
past the extension lines</source>
      <translation>Ölçülendirme çizgisinin uzantılarının 
geçtiği mesafe</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="269"/>
      <source>Length of the extension lines</source>
      <translation>Uzatma çizgilerinin uzunluğu</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="279"/>
      <source>Length of the extension line
beyond the dimension line</source>
      <translation>Uzatma çizgisinin ölçü çizgisinin
dışındaki mesafesi</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="288"/>
      <source>Shows the dimension line and arrows</source>
      <translation>Ölçülendirme çizgisi ve okları gösterir</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="67"/>
      <source>If it is true, the objects contained within this layer will adopt the line color of the layer</source>
      <translation>Doğruysa, bu katmanda bulunan nesneler katmanın çizgi rengini alacaktır</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="78"/>
      <source>If it is true, the objects contained within this layer will adopt the shape color of the layer</source>
      <translation>Doğruysa, bu katmanda bulunan nesneler katmanın çizgi rengini alacaktır</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="89"/>
      <source>If it is true, the print color will be used when objects in this layer are placed on a TechDraw page</source>
      <translation>Doğruysa, bu katmandaki nesneler bir TechDraw sayfasına yerleştirildiğinde baskı rengi kullanılacaktır</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="103"/>
      <source>The line color of the objects contained within this layer</source>
      <translation>Bu katmanda bulunan nesnelerin çizgi rengi</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="117"/>
      <source>The shape color of the objects contained within this layer</source>
      <translation>Bu katmanda bulunan nesnelerin şekil rengi</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="131"/>
      <source>The line width of the objects contained within this layer</source>
      <translation>Bu katmanda bulunan nesnelerin çizgi kalınlığı</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="143"/>
      <source>The draw style of the objects contained within this layer</source>
      <translation>Bu katmanda bulunan nesnelerin çizim biçimi</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="154"/>
      <source>The transparency of the objects contained within this layer</source>
      <translation>Bu katmanda bulunan nesnelerin şeffaflığı</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="165"/>
      <source>The line color of the objects contained within this layer, when used on a TechDraw page</source>
      <translation>TechDraw sayfasında kullanıldığında, bu katmanda bulunan nesnelerin çizgi rengi</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_base.py" line="106"/>
      <source>Defines an SVG pattern.</source>
      <translation>Bir SVG deseni tanımlar.</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_base.py" line="116"/>
      <source>Defines the size of the SVG pattern.</source>
      <translation>SVG modelinin boyutunu tanımlar.</translation>
    </message>
  </context>
  <context>
    <name>Dialog</name>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="14"/>
      <source>Annotation Styles Editor</source>
      <translation>Ek Açıklama Stil Düzenleyicisi</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="20"/>
      <source>Style name</source>
      <translation>Stil adı</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="38"/>
      <source>The name of your style. Existing style names can be edited.</source>
      <translation>Tekniğinizi isimlendirin. Haricen teknik isimler düzenlenebilir.</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="50"/>
      <source>Add new...</source>
      <translation>Yeni ekle...</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="79"/>
      <source>Renames the selected style</source>
      <translation>Seçili öğeyi yeniden adlandırır</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="82"/>
      <source>Rename</source>
      <translation>Yeniden Adlandır</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="110"/>
      <source>Deletes the selected style</source>
      <translation>Seçili ögeyi siler</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="113"/>
      <source>Delete</source>
      <translation>Sil</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="120"/>
      <source>Import styles from json file</source>
      <translation>Stiller içeriye Json JavaScript Object Notation dosyasından</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="130"/>
      <source>Export styles to json file</source>
      <translation>Stiller dışarıya Json JavaScript Object Notation dosyasına</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="164"/>
      <source>Text</source>
      <translation>Metin</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="170"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="192"/>
      <source>The font to use for texts and dimensions</source>
      <translation>Metinler ve boyutlar için kullanılacak yazı tipi</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="173"/>
      <source>Font name</source>
      <translation>Yazı tipi ismi</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="199"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="209"/>
      <source>Font size in the system units</source>
      <translation>Birim sistemlerindeki yazı tipi boyutu</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="202"/>
      <source>Font size</source>
      <translation>Yazı Boyutu</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="219"/>
      <source>Line spacing in system units</source>
      <translation>Sistem birimlerinde satır aralığı</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="222"/>
      <source>Line spacing</source>
      <translation>Satır aralığı</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="239"/>
      <source>Units</source>
      <translation>Birimler</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="245"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="255"/>
      <source>A multiplier factor that affects the size of texts and markers</source>
      <translation>Metinlerin ve işaretleyicilerin boyutuna izin veren bir çoklayıcı etkeni</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="248"/>
      <source>Scale multiplier</source>
      <translation>Ölçek çarpanı</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="268"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="278"/>
      <source>If it is checked it will show the unit next to the dimension value</source>
      <translation>O seçilmişse birim değerini ölçü değerinin yanında gösterir</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="271"/>
      <source>Show unit</source>
      <translation>Birimi göster</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="291"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="301"/>
      <source>Specify a valid length unit like mm, m, in, ft, to force displaying the dimension value in this unit</source>
      <translation>Bu birimin ölçü değerini göstermeye zorlamak için mm, m, in, ft gibi geçerli bir uzunluğu seçin</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="294"/>
      <source>Unit override</source>
      <translation>Birim geçersiz kılma</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="308"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="318"/>
      <source>The number of decimals to show for dimension values</source>
      <translation>Ölçü değerlerini hassas göstermek için ondalıkların sayısı</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="311"/>
      <source>Decimals</source>
      <translation>Ondalıklar</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="331"/>
      <source>Line and arrows</source>
      <translation>Çizgi ve Oklar</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="337"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="347"/>
      <source>If it is checked it will display the dimension line</source>
      <translation>O seçildiyse ölçülendirme çizgisini gösterir</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="340"/>
      <source>Show lines</source>
      <translation>Çizgileri göster</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="363"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="373"/>
      <source>The width of the dimension lines</source>
      <translation>Ölçülendirme çizgisinin kalınlığı</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="366"/>
      <source>Line width</source>
      <translation>Çizgi Kalınlığı</translation>
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
      <translation>Ölçülendirme çizgilerinin, okların ve metinlerin rengi</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="389"/>
      <source>Line / text color</source>
      <translation>Çizgi / metin rengi</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="410"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="432"/>
      <source>The type of arrows or markers to use at the end of dimension lines</source>
      <translation>Ölçülendirme çizgilerinin sonunda kullanmak için okların veya işaretleyicilerin tipi</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="413"/>
      <source>Arrow type</source>
      <translation>Ok tipi</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="436"/>
      <source>Dot</source>
      <translation>Nokta</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="441"/>
      <source>Circle</source>
      <translation>Daire</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="446"/>
      <source>Arrow</source>
      <translation>Ok İşareti</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="451"/>
      <source>Tick</source>
      <translation>işaretleme</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="456"/>
      <source>Tick-2</source>
      <translation>Tik-2</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="464"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="474"/>
      <source>The size of the dimension arrows or markers in system units</source>
      <translation>Birim sistemlerindeki büyüklük oklarının veya işaretleyicilerin boyutu</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="467"/>
      <source>Arrow size</source>
      <translation>Ok boyu</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="484"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="494"/>
      <source>The distance that the dimension line is additionally extended</source>
      <translation>Mesafeyi ifade eden ölçülendirme çizgisi ek olarak uzatılır</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="487"/>
      <source>Dimension overshoot</source>
      <translation>Ölçülendirme çizgisi aşımı</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="504"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="514"/>
      <source>The length of the extension lines</source>
      <translation>Uzatma çizgilerinin uzunluğu</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="507"/>
      <source>Extension lines</source>
      <translation>Uzatma çizgileri</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="524"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="534"/>
      <source>The distance that the extension lines are additionally extended beyond the dimension line</source>
      <translation>Mesafeyi ifade eden uzama çizgileri ölçülendirme çizgisinin ötesine eklenerek uzatılır</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="527"/>
      <source>Extension overshoot</source>
      <translation>Uzatma çizgisi aşımı</translation>
    </message>
  </context>
  <context>
    <name>Draft</name>
    <message>
      <location filename="../../importDXF.py" line="129"/>
      <source>Download of dxf libraries failed.
Please install the dxf Library addon manually
from menu Tools -&gt; Addon Manager</source>
      <translation>Dxf kitaplıklarının indirilmesi başarısız oldu.
Lütfen dxf Kütüphane addon'unu manuel olarak kurun
menüden Araçlar -&gt; Eklenti Yöneticisi</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="133"/>
      <location filename="../../InitGui.py" line="134"/>
      <location filename="../../InitGui.py" line="135"/>
      <location filename="../../InitGui.py" line="136"/>
      <location filename="../../InitGui.py" line="137"/>
      <source>Draft</source>
      <translation>Taslak</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="179"/>
      <location filename="../../InitGui.py" line="180"/>
      <location filename="../../InitGui.py" line="181"/>
      <location filename="../../InitGui.py" line="182"/>
      <source>Import-Export</source>
      <translation>İçe-Dışa Aktar</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="248"/>
      <source>Toggles Grid On/Off</source>
      <translation>Izgara görünümünü Aç/Kapat</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="265"/>
      <source>Object snapping</source>
      <translation>Nesne Yakalama</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="297"/>
      <source>Toggles Visual Aid Dimensions On/Off</source>
      <translation>Görsel Yardım Ölçülerini Açar / Kapatır</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="316"/>
      <source>Toggles Ortho On/Off</source>
      <translation>Izgara görünümünü Aç/Kapat</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="334"/>
      <source>Toggles Constrain to Working Plane On/Off</source>
      <translation>Çalışma Düzlemine Kısıtlamayı Açma / Kapatma</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="106"/>
      <source>_BSpline.createGeometry: Closed with same first/last Point. Geometry not updated.</source>
      <translation>_BSpline.createGeometry: Aynı ilk / son nokta ile kapatıldı. Geometri güncellenmedi.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="244"/>
      <location filename="../../draftobjects/pointarray.py" line="306"/>
      <source>Point object doesn't have a discrete point, it cannot be used for an array.</source>
      <translation>Nokta nesnesinin ayrık bir noktası yoktur, bir dizi (array) için kullanılamaz.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="88"/>
      <location filename="../../draftguitools/gui_lineslope.py" line="91"/>
      <source>Slope</source>
      <translation>Eğim</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="91"/>
      <source>Clone</source>
      <translation>Klon</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="49"/>
      <source>You must choose a base object before using this command</source>
      <translation>Bu komutu kullanmadan önce temel alınacak bir nesne seçmelisiniz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="89"/>
      <source>Delete original objects</source>
      <translation>Orijinal nesneleri silin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="95"/>
      <source>Create chamfer</source>
      <translation>Pah oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="274"/>
      <source>Save style</source>
      <translation>Biçimi kaydet</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="275"/>
      <source>Name of this new style:</source>
      <translation>Yeni biçimin ismi:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="282"/>
      <source>Warning</source>
      <translation>Uyarı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="283"/>
      <source>Name exists. Overwrite?</source>
      <translation>Bu isim kullanılıyor. Üzerine yazılsın mı?</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="318"/>
      <source>Error: json module not found. Unable to save style</source>
      <translation>Hata: json modülü bulunamadı. Biçim kaydedilemedi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="329"/>
      <source>Offset direction is not defined. Please move the mouse on either side of the object first to indicate a direction</source>
      <translation>Ofset yönü tanımlanmadı. Bir yönü belirtmek için lütfen fareyi nesnenin her iki yanında hareket ettirin</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="144"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="156"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="157"/>
      <source>True</source>
      <translation>Doğru</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="148"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="160"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="159"/>
      <source>False</source>
      <translation>Yanlış</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="151"/>
      <source>Scale</source>
      <translation>Ölçek</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="152"/>
      <source>X factor</source>
      <translation>X faktörü</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="153"/>
      <source>Y factor</source>
      <translation>Y faktörü</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="154"/>
      <source>Z factor</source>
      <translation>Z faktörü</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="155"/>
      <source>Uniform scaling</source>
      <translation>Tekdüze Ölçekleme</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="156"/>
      <source>Working plane orientation</source>
      <translation>Çalışma düzlemi yönlendirmesi</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="157"/>
      <source>Copy</source>
      <translation>Kopyala</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="158"/>
      <source>Modify subelements</source>
      <translation>AltElemanları değiştir</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="159"/>
      <source>Pick from/to points</source>
      <translation>Noktalar ı/dan seçin</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="160"/>
      <source>Create a clone</source>
      <translation>Klon oluştur</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_wpproxy.py" line="102"/>
      <source>Writing camera position</source>
      <translation>Yazma kamera konumu</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_wpproxy.py" line="119"/>
      <source>Writing objects shown/hidden state</source>
      <translation>Gösterilen / gizli durumdaki nesneleri yazma</translation>
    </message>
  </context>
  <context>
    <name>DraftCircularArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="26"/>
      <source>Circular array</source>
      <translation>Dairesel dizi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(Placeholder for the icon)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="53"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="63"/>
      <source>Distance from one layer of objects to the next layer of objects.</source>
      <translation>Nesnelerin bir katmanından nesnelerin sonraki katmanına mesafe.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="56"/>
      <source>Radial distance</source>
      <translation>Dairesel çap uzunluğu</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="76"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="87"/>
      <source>Distance from one element in one ring of the array to the next element in the same ring.
It cannot be zero.</source>
      <translation>Dizinin bir halkasındaki bir elemandan aynı halkadaki sonraki elemana mesafe.
O sıfır değerinde olamaz.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="80"/>
      <source>Tangential distance</source>
      <translation>Eğrisel mesafe</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="101"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="112"/>
      <source>Number of circular layers or rings to create, including a copy of the original object.
It must be at least 2.</source>
      <translation>Number of circular layers or rings to create, including a copy of the original object.
It must be at least 2.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="105"/>
      <source>Number of circular layers</source>
      <translation>Dairesel katmanların sayısı</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="126"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="136"/>
      <source>The number of symmetry lines in the circular array.</source>
      <translation>Dairesel dizideki simetrik çizgilerin sayısı.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="129"/>
      <source>Symmetry</source>
      <translation>Simetri</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="151"/>
      <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
      <translation>Dönme eksenleri olanları ifade eden noktanın koordinatları geçer.
Özellik düzenleyicide ekselerin kendilerine ait yönleri değiştirin.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="155"/>
      <source>Center of rotation</source>
      <translation>Döndürme merkezi</translation>
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
      <translation>Döndürme merkezi koordinatlarını sıfırla.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="228"/>
      <source>Reset point</source>
      <translation>Noktayı sıfırla</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="240"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Seçilmişse, birbirlerine değiyorlarsa dizideki sonuçlanan nesneler yapışırlar.
Bu sadece "bağlantı" kapalıysa çalışır.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="244"/>
      <source>Fuse</source>
      <translation>Birleştir</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="251"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Seçilmişse sonuçlanan nesne düzenli bir dizi yerine bir bağlantı olacaktır.
Birden fazla kopyalar oluştuğu zaman bir bağlantı daha verimlidir ama o, birbirlerini yapıştıramaz.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="255"/>
      <source>Link array</source>
      <translation>Bağlantı dizisi</translation>
    </message>
  </context>
  <context>
    <name>DraftOrthoArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="26"/>
      <source>Orthogonal array</source>
      <translation>Dikey dizi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(Placeholder for the icon)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="51"/>
      <source>Number of elements in the array in the specified direction, including a copy of the original object.
The number must be at least 1 in each direction.</source>
      <translation>Orijinal nesnenin bir kopyası dahil, dizide belirtilen yöndeki öğelerin sayısı.
Sayı her yönde en az 1 olmalıdır.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="55"/>
      <source>Number of elements</source>
      <translation>Unsur sayısı</translation>
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
      <translation>X yönündeki öğelerin aralarındaki uzaklık.
Normalde, sadece X değeri gereklidir. Diğer iki değer kendi ilgili yönlerinde ek kaymaları sağlayabilir.
Negatif değerler kopyaların negatif yönde üretilmesine neden olacaktır.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="124"/>
      <source>X intervals</source>
      <translation>X Aralıkları</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="197"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="288"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="379"/>
      <source>Reset the distances.</source>
      <translation>Mesafeleri sıfırla.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="200"/>
      <source>Reset X</source>
      <translation>Sıfırla X</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="210"/>
      <source>Distance between the elements in the Y direction.
Normally, only the Y value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Y yönündeki öğelerin aralarındaki uzaklık.
Normalde, sadece Y değeri gereklidir. Diğer iki değer kendi ilgili yönlerinde ek kaymaları sağlayabilir.
Negatif değerler kopyaların negatif yönde üretilmesine neden olacaktır.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="215"/>
      <source>Y intervals</source>
      <translation>Y Aralıkları</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="291"/>
      <source>Reset Y</source>
      <translation>Sıfırla Y</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="301"/>
      <source>Distance between the elements in the Z direction.
Normally, only the Z value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Z yönündeki öğelerin aralarındaki uzaklık.
Normalde, sadece Z değeri gereklidir. Diğer iki değer kendi ilgili yönlerinde ek kaymaları sağlayabilir.
Negatif değerler kopyaların negatif yönde üretilmesine neden olacaktır.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="306"/>
      <source>Z intervals</source>
      <translation>Z Aralıkları</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="382"/>
      <source>Reset Z</source>
      <translation>Z'yi sıfırla</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="394"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Seçilmişse, birbirlerine değiyorlarsa dizideki sonuçlanan nesneler yapışırlar.
Bu sadece "bağlantı" kapalıysa çalışır.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="398"/>
      <source>Fuse</source>
      <translation>Birleştir</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="405"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Seçilmişse sonuçlanan nesne düzenli bir dizi yerine bir bağlantı olacaktır.
Birden fazla kopyalar oluştuğu zaman bir bağlantı daha verimlidir ama o, birbirlerini yapıştıramaz.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="409"/>
      <source>Link array</source>
      <translation>Bağlantı dizisi</translation>
    </message>
  </context>
  <context>
    <name>DraftPolarArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="26"/>
      <source>Polar array</source>
      <translation>Kutupsal dizi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(Placeholder for the icon)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="53"/>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="65"/>
      <source>Sweeping angle of the polar distribution.
A negative angle produces a polar pattern in the opposite direction.
The maximum absolute value is 360 degrees.</source>
      <translation>Sweeping angle of the polar distribution.
A negative angle produces a polar pattern in the opposite direction.
The maximum absolute value is 360 degrees.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="58"/>
      <source>Polar angle</source>
      <translation>Kutupsal açı</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="86"/>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="97"/>
      <source>Number of elements in the array, including a copy of the original object.
It must be at least 2.</source>
      <translation>Orijinal nesnenin bir kopyası dahil olmak üzere dizideki öğelerin sayısı.
En az 2 olmalıdır.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="90"/>
      <source>Number of elements</source>
      <translation>Unsur sayısı</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="113"/>
      <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
      <translation>Dönme eksenleri olanları ifade eden noktanın koordinatları geçer.
Özellik düzenleyicide ekselerin kendilerine ait yönleri değiştirin.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="117"/>
      <source>Center of rotation</source>
      <translation>Döndürme merkezi</translation>
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
      <translation>Döndürme merkezi koordinatlarını sıfırla.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="190"/>
      <source>Reset point</source>
      <translation>Noktayı sıfırla</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="202"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>Seçilmişse, birbirlerine değiyorlarsa dizideki sonuçlanan nesneler yapışırlar.
Bu sadece "bağlantı" kapalıysa çalışır.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="206"/>
      <source>Fuse</source>
      <translation>Birleştir</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="213"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>Seçilmişse sonuçlanan nesne düzenli bir dizi yerine bir bağlantı olacaktır.
Birden fazla kopyalar oluştuğu zaman bir bağlantı daha verimlidir ama o, birbirlerini yapıştıramaz.</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="217"/>
      <source>Link array</source>
      <translation>Bağlantı dizisi</translation>
    </message>
  </context>
  <context>
    <name>DraftShapeStringGui</name>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="26"/>
      <source>ShapeString</source>
      <translation>Şekil dizesi</translation>
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
      <translation>Koordinatları girin veya noktayı fareyle seçin.</translation>
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
      <translation>3B nokta seçimini sıfırla</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="120"/>
      <source>Reset Point</source>
      <translation>Noktayı sıfırla</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="131"/>
      <source>String</source>
      <translation>Dize</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="138"/>
      <source>Text to be made into ShapeString</source>
      <translation>Şekil Dizesine (ShapeString) çevrilecek metin</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="149"/>
      <source>Height</source>
      <translation>Yükseklik</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="156"/>
      <source>Height of the result</source>
      <translation>Sonucun yüksekliği</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="176"/>
      <source>Font file</source>
      <translation>Yazı tipi dosyası</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddConstruction</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="309"/>
      <source>Add to Construction group</source>
      <translation>Yapı grubuna ekle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="312"/>
      <source>Adds the selected objects to the construction group,
and changes their appearance to the construction style.
It creates a construction group if it doesn't exist.</source>
      <translation>Seçilen nesneleri inşaat grubuna ekler,
ve görünüşlerini inşaat tarzına değiştirir.
Yoksa bir inşaat grubu oluşturur.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddNamedGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="361"/>
      <source>Add a new named group</source>
      <translation>Yeni bir adlandırılmış grup ekle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="365"/>
      <source>Add a new group with a given name.</source>
      <translation>Verilen isimle yeni bir grup ekle.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddPoint</name>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="57"/>
      <source>Add point</source>
      <translation>Nokta ekle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="60"/>
      <source>Adds a point to an existing Wire or B-spline.</source>
      <translation>Mevcut bir Tel veya B-spline'a bir nokta ekler.</translation>
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
      <translation>Seçili nesneleri mevcut bir gruba taşır veya herhangi bir gruptan kaldırır.
Bu aracı kullanmak için önce bir grup oluşturun.</translation>
    </message>
  </context>
  <context>
    <name>Draft_AnnotationStyleEditor</name>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="78"/>
      <source>Annotation styles...</source>
      <translation>Açıklama biçimleri...</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="82"/>
      <source>Manage or create annotation styles</source>
      <translation>Ek açıklama biçimini oluşturun ya da yönetin</translation>
    </message>
  </context>
  <context>
    <name>Draft_ApplyStyle</name>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="47"/>
      <source>Apply current style</source>
      <translation>Geçerli biçimi uygula</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="50"/>
      <source>Applies the current style defined in the toolbar (line width and colors) to the selected objects and groups.</source>
      <translation>Araç çubuğunda tanımlanan mevcut biçimi (çizgi kalınlığı ve renkleri) seçili nesnelere ve gruplara uygular.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Arc</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="66"/>
      <source>Arc</source>
      <translation>Yay</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="69"/>
      <source>Creates a circular arc by a center point and a radius.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Bir merkez noktası ve bir yarıçapla dairesel bir yay oluşturur.
Nokta yakalamak için CTRL, sınırlamak için SHIFT.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ArcTools</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="606"/>
      <source>Arc tools</source>
      <translation>Yay Araçları</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="609"/>
      <source>Create various types of circular arcs.</source>
      <translation>Çeşitli türlerde dairesel yaylar oluşturun.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Arc_3Points</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="487"/>
      <source>Arc by 3 points</source>
      <translation>3 Noktalı Yay</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="490"/>
      <source>Creates a circular arc by picking 3 points.
CTRL to snap, SHIFT to constrain.</source>
      <translation>3 nokta seçerek dairesel bir yay oluşturur.
Yakalamak için CTRL, kısıtlamak için SHIFT.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Array</name>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="68"/>
      <source>Array</source>
      <translation>Dizi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="71"/>
      <source>Creates an array from a selected object.
By default, it is a 2x2 orthogonal array.
Once the array is created its type can be changed
to polar or circular, and its properties can be modified.</source>
      <translation>Seçilen bir nesneden bir dizi oluşturur.
Varsayılan olarak, 2x2 ortogonal dizidir.
Dizi oluşturulduktan sonra, özellikleri ve türü 
(kutupsal veya dairesel) olarak değiştirilebilir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ArrayTools</name>
    <message>
      <location filename="../../draftguitools/gui_arrays.py" line="65"/>
      <source>Array tools</source>
      <translation>Sıralama araçları</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arrays.py" line="68"/>
      <source>Create various types of arrays, including rectangular, polar, circular, path, and point</source>
      <translation>Dikdörtgen, kutupsal, dairesel, yol ve nokta dahil olmak üzere çeşitli diziler oluşturun</translation>
    </message>
  </context>
  <context>
    <name>Draft_AutoGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="208"/>
      <source>Autogroup</source>
      <translation>Otogrup</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="211"/>
      <source>Select a group to add all Draft and Arch objects to.</source>
      <translation>Tüm Taslak ve Mimari nesnelerinin ekleneceği bir grup seçin.</translation>
    </message>
  </context>
  <context>
    <name>Draft_BSpline</name>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="60"/>
      <source>B-spline</source>
      <translation>B-Şerit</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="63"/>
      <source>Creates a multiple-point B-spline. CTRL to snap, SHIFT to constrain.</source>
      <translation>Bir çok-noktalı B-spline oluşturur. Nokta Yakalamak için CTRL, kısıtlamak için SHIFT.</translation>
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
      <translation>Daire</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_circles.py" line="84"/>
      <source>Creates a circle (full circular arc).
CTRL to snap, ALT to select tangent objects.</source>
      <translation>Bir daire (tam dairesel yay) oluşturur.
Nokta yakalamak için CTRL, teğet nesneleri seçmek için ALT.</translation>
    </message>
  </context>
  <context>
    <name>Draft_CircularArray</name>
    <message>
      <location filename="../../draftguitools/gui_circulararray.py" line="65"/>
      <source>Circular array</source>
      <translation>Dairesel dizi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_circulararray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in a radial pattern
creating various circular layers.

The array can be turned into an orthogonal or a polar array by changing its type.</source>
      <translation>Seçilen nesnenin kopyalarını oluşturur ve kopyaları radyal bir modele yerleştirir
çeşitli dairesel katmanlar oluşturur.

Dizi, türü değiştirilerek ortogonal veya kutupsal bir diziye dönüştürülebilir.</translation>
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
      <translation>Seçilen nesnelerin bir klonunu oluşturur.
Ortaya çıkan klon, üç yönünün her birinde ölçeklenebilir.</translation>
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
      <translation>Noktayı Kaldır</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="92"/>
      <source>Removes a point from an existing Wire or B-spline.</source>
      <translation>Mevcut bir Tel veya B-spline'dan bir noktayı kaldırır.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Dimension</name>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="84"/>
      <source>Dimension</source>
      <translation>Boyut</translation>
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
      <translation>Bir boyut oluşturur.

- Basit bir doğrusal boyut oluşturmak için üç nokta seçin.
- O çizgiye bağlı doğrusal bir boyut oluşturmak için düz bir çizgi seçin.
- O yaya bağlı bir yarıçap veya çap ölçümlendirmesi oluşturmak için bir yay 
veya daire seçin.
- Aralarında açısal bir boyut oluşturmak için iki düz çizgi seçin.
Nokta yakalamak için CTRL, sınırlamak için SHIFT, kenar veya yay seçmek için ALT.

Karşılık gelen bağlantılı ölçümlendirmeyi oluşturmak için bu komutu başlatmadan
 önce tek bir çizgi veya tek bir dairesel yay seçebilirsiniz.
Ayrıca, bu komutu başlatmadan önce onu bir 'Taslak Ölçüsü' nesnesine dönüştürmek 
için bir 'App:: MeasureDistance' nesnesi de seçebilirsiniz.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Downgrade</name>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="58"/>
      <source>Downgrade</source>
      <translation>İndirge</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="61"/>
      <source>Downgrades the selected objects into simpler shapes.
The result of the operation depends on the types of objects, which may be able to be downgraded several times in a row.
For example, it explodes the selected polylines into simpler faces, wires, and then edges. It can also subtract faces.</source>
      <translation>Seçili nesneleri daha basit şekillere indirger.
İşlemin sonucu, arka arkaya birkaç kez düşürülebilen nesne türlerine bağlıdır.
Örneğin, seçilen çoklu çizgileri daha basit yüzeylere, tellere ve ardından kenarlara patlatır. Yüzleri de çıkarabilir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Draft2Sketch</name>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="57"/>
      <source>Draft to Sketch</source>
      <translation>Taslaktan Eskize</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="60"/>
      <source>Convert bidirectionally between Draft objects and Sketches.
Many Draft objects will be converted into a single non-constrained Sketch.
However, a single sketch with disconnected traces will be converted into several individual Draft objects.</source>
      <translation>Taslak (Draft) nesneleri ve Eskizler (Sketches) arasında çift yönlü olarak dönüştürün.
Birçok Taslak nesnesi, sınırlandırılmamış tek bir Eskiz'e dönüştürülecektir.
Ancak, bağlantısı kesilmiş izlere sahip tek bir çizim birkaç ayrı Taslak nesnesine dönüştürülecektir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Drawing</name>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="71"/>
      <source>Drawing</source>
      <translation>Çizim</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="74"/>
      <source>Creates a 2D projection on a Drawing Workbench page from the selected objects.
This command is OBSOLETE since the Drawing Workbench became obsolete in 0.17.
Use TechDraw Workbench instead for generating technical drawings.</source>
      <translation>Bir Çizim Tezgahı (Drawing Workbench) sayfasında seçilen nesnelerden bir 2B projeksiyon oluşturur.
Çizim Tezgahı (Drawing WB) 0.17'de kullanılmadığından bu komut KULLANIM DIŞI'dir.
Teknik çizimler oluşturmak için TechDraw Çalışma Tezgahını kullanın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Edit</name>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="275"/>
      <source>Edit</source>
      <translation>Düzenle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="284"/>
      <source>Edits the active object.
Press E or ALT+LeftClick to display context menu
on supported nodes and on supported objects.</source>
      <translation>Aktif nesneyi düzenler.
Desteklenen düğümlerde ve desteklenen nesnelerde içerik 
menüsünü görüntülemek için E veya ALT + FareninSOL tuşlarına basın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Ellipse</name>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="59"/>
      <source>Ellipse</source>
      <translation>Elips</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="62"/>
      <source>Creates an ellipse. CTRL to snap.</source>
      <translation>Bir elips oluşturur. Nesne yakalamak için CTRL.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Facebinder</name>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="60"/>
      <source>Facebinder</source>
      <translation>YüzeyBağlayıcı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="62"/>
      <source>Creates a facebinder object from selected faces.</source>
      <translation>Seçili yüzeylerden bir yüz bağlayıcı (Facebinder) nesnesi oluşturur.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Fillet</name>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="64"/>
      <source>Fillet</source>
      <translation>Yuvarla</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="67"/>
      <source>Creates a fillet between two selected wires or edges.</source>
      <translation>Seçili iki çizgi arasında yarıçapı girilen bir yay oluşturur.</translation>
    </message>
  </context>
  <context>
    <name>Draft_FlipDimension</name>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="62"/>
      <source>Flip dimension</source>
      <translation>Ölçüyü ters çevir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="66"/>
      <source>Flip the normal direction of the selected dimensions (linear, radial, angular).
If other objects are selected they are ignored.</source>
      <translation>Seçili boyutların normal yönünü çevirin (doğrusal, radyal, açısal).
Diğer nesneler seçilirse, bunlar göz ardı edilir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Hatch</name>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="38"/>
      <source>Hatch</source>
      <translation>Tarama</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="42"/>
      <source>Creates hatches on the faces of a selected object</source>
      <translation>Creates hatches on the faces of a selected object</translation>
    </message>
  </context>
  <context>
    <name>Draft_Heal</name>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="58"/>
      <source>Heal</source>
      <translation>İyileştir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="61"/>
      <source>Heal faulty Draft objects saved with an earlier version of the program.
If an object is selected it will try to heal that object in particular,
otherwise it will try to heal all objects in the active document.</source>
      <translation>Programın önceki bir sürümüyle kaydedilen hatalı Taslak nesnelerini iyileştirin.
Bir nesne seçilirse, özellikle o nesneyi iyileştirmeye çalışır,
aksi takdirde aktif belgedeki tüm nesneleri iyileştirmeye çalışır.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Join</name>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="66"/>
      <source>Join</source>
      <translation>Birleştir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="69"/>
      <source>Joins the selected lines or polylines into a single object.
The lines must share a common point at the start or at the end for the operation to succeed.</source>
      <translation>Seçili çizgileri veya çoklu çizgileri tek bir nesnede birleştirir.
İşlemin başarılı olması için çizgilerin başında veya sonunda ortak bir noktayı paylaşması gerekir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Label</name>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="64"/>
      <source>Label</source>
      <translation>Etiket</translation>
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
      <translation>İsteğe bağlı olarak seçilen bir nesneye veya alt öğeye eklenmiş bir etiket oluşturur.

Önce bir nesnenin noktasını, kenarını veya yüzeyini seçin, ardından bu komutu çağırın 
ve ardından kılavuz çizginin ve metin etiketinin konumunu ayarlayın.
Etiket, bu nesne ve varsa seçilen alt eleman hakkında bilgi görüntüleyebilecektir.

Çok sayıda nesne veya birçok alt öğe seçilirse, etikete bilgi sağlamak için her durumda
yalnızca birincisi kullanılacaktır.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Layer</name>
    <message>
      <location filename="../../draftguitools/gui_layers.py" line="52"/>
      <source>Layer</source>
      <translation>Katman</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_layers.py" line="55"/>
      <source>Adds a layer to the document.
Objects added to this layer can share the same visual properties such as line color, line width, and shape color.</source>
      <translation>Belgeye bir katman ekler.
Bu katmana eklenen nesneler, çizgi rengi, çizgi kalınlığı ve şekil rengi gibi aynı görsel özellikleri paylaşabilir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Line</name>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="63"/>
      <source>Line</source>
      <translation>Çizgi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="66"/>
      <source>Creates a 2-point line. CTRL to snap, SHIFT to constrain.</source>
      <translation>2 noktalı bir çizgi oluşturur. Nokta Yakalamak için CTRL, kısıtlamak için SHIFT.</translation>
    </message>
  </context>
  <context>
    <name>Draft_LinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="119"/>
      <source>LinkArray</source>
      <translation>BağlantıDizisi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="122"/>
      <source>Like the Array tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>Dizi aracı gibi, ancak bunun yerine bir 'Bağlantı dizisi' oluşturur.
Bir 'Bağlantı dizisi', birçok kopyayı işlerken daha etkilidir, ancak 'Birleştir (Fuse)' seçeneği kullanılamaz.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Mirror</name>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="63"/>
      <source>Mirror</source>
      <translation>Aynala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="66"/>
      <source>Mirrors the selected objects along a line defined by two points.</source>
      <translation>Seçili nesneleri iki nokta ile tanımlanan bir çizgi boyunca yansıtır. Ayna görüntüsünü oluşturur.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Move</name>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="63"/>
      <source>Move</source>
      <translation>Taşı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="66"/>
      <source>Moves the selected objects from one base point to another point.
If the "copy" option is active, it will create displaced copies.
CTRL to snap, SHIFT to constrain.</source>
      <translation>Seçili nesneleri bir temel noktadan başka bir noktaya taşır.
"Kopyala" seçeneği işaretlenirse, yeri değiştirilmiş kopyalar oluşturacaktır.
Yakalamak için CTRL, sınırlamak için SHIFT.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Offset</name>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="63"/>
      <source>Offset</source>
      <translation>Uzaklaşma</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="66"/>
      <source>Offsets of the selected object.
It can also create an offset copy of the original object.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
      <translation>Seçili nesnenin ofsetini oluşturur. 
Orijinal nesnenin bir ofset kopyasını da oluşturabilir.
Yakalamak için CTRL, sınırlamak için SHIFT. Her tıklamayla bir kopya oluşturmak için ALT tuşunu basılı tutun ve tıklayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_OrthoArray</name>
    <message>
      <location filename="../../draftguitools/gui_orthoarray.py" line="65"/>
      <source>Array</source>
      <translation>Dizi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_orthoarray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in an orthogonal pattern,
meaning the copies follow the specified direction in the X, Y, Z axes.

The array can be turned into a polar or a circular array by changing its type.</source>
      <translation>Seçilen nesnenin kopyalarını oluşturur ve kopyaları ortogonal bir modele yerleştirir,
kopyaların X, Y, Z eksenlerinde belirtilen yönü takip ettiği anlamına gelir.

Dizi, türü değiştirilerek kutupsal veya dairesel bir diziye dönüştürülebilir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathArray</name>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="73"/>
      <source>Path array</source>
      <translation>Nesneyi bir yol dizisine göre kopyalar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="76"/>
      <source>Creates copies of the selected object along a selected path.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
      <translation>Seçilen bir yol boyunca seçilen nesnenin kopyalarını oluşturur.
Önce nesneyi seçin ve ardından yolu seçin.
Yol bir çoklu çizgi, B-spline, Bezier eğrisi veya diğer nesnelerin kenarları olabilir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="163"/>
      <source>Path Link array</source>
      <translation>Yol Bağlantısı dizisi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="166"/>
      <source>Like the PathArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>Dizi aracı gibi, ancak bunun yerine bir 'Bağlantı dizisi' oluşturur.
Bir 'Bağlantı dizisi', birçok kopyayı işlerken daha etkilidir, ancak 'Birleştir (Fuse)' seçeneği kullanılamaz.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathTwistedArray</name>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="66"/>
      <source>Path twisted array</source>
      <translation>Yol bükülmüş dizi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="69"/>
      <source>Creates copies of the selected object along a selected path, and twists the copies.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
      <translation>Seçilen bir yol boyunca seçilen nesnenin kopyalarını oluşturur ve kopyaları büker.
Önce nesneyi seçin ve ardından yolu seçin.
Yol bir çoklu çizgi, B-spline, Bezier eğrisi veya diğer nesnelerin kenarları olabilir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathTwistedLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="121"/>
      <source>Path twisted Link array</source>
      <translation>Yol bükülmüş Bağlantı dizisi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="124"/>
      <source>Like the PathTwistedArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>PathTwistedArray aracı gibi, ancak bunun yerine bir 'Bağlantı dizisi' oluşturur.
Birçok kopyayı işlerken bir 'Bağlantı dizisi' daha etkilidir ancak 'fuse' seçeneği kullanılamaz.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Point</name>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="62"/>
      <source>Point</source>
      <translation>Nokta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="65"/>
      <source>Creates a point object. Click anywhere on the 3D view.</source>
      <translation>Bir nokta nesnesi oluşturur. 3B görünümde herhangi bir yere tıklayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PointArray</name>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="76"/>
      <source>Point array</source>
      <translation>Noktalara göre dizi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="79"/>
      <source>Creates copies of the selected object, and places the copies at the position of various points.

The points need to be grouped under a compound of points before using this tool.
To create this compound, select various points and then use the Part Compound tool,
or use the Draft Upgrade tool to create a 'Block', or create a Sketch and add simple points to it.

Select the base object, and then select the compound or the sketch to create the point array.</source>
      <translation>Seçilen nesnenin kopyalarını oluşturur ve kopyaları çeşitli noktaların konumuna yerleştirir.

Bu aracı kullanmadan önce noktaların bir bileşik nokta altında gruplanması gerekir.
Bu bileşiği oluşturmak için çeşitli noktaları seçin ve ardından Parça Bileşik (part compount) aracını kullanın,
veya bir 'Blok' oluşturmak için Taslak Yükseltme aracını kullanın veya bir Çizim oluşturun ve ona basit noktalar ekleyin.

Temel nesneyi seçin ve ardından nokta dizisini oluşturmak için bileşiği veya çizimi seçin.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PointLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="141"/>
      <source>PointLinkArray</source>
      <translation>Noktalar bağlantı dizgi oluşturma</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="144"/>
      <source>Like the PointArray tool, but creates a 'Point link array' instead.
A 'Point link array' is more efficient when handling many copies.</source>
      <translation>PointArray aracı gibi, ancak bunun yerine bir 'Point link dizisi' oluşturur.
Birçok kopyayı işlerken 'Nokta bağlantı dizisi' daha etkilidir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_PolarArray</name>
    <message>
      <location filename="../../draftguitools/gui_polararray.py" line="65"/>
      <source>Polar array</source>
      <translation>Kutupsal dizi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polararray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in a polar pattern
defined by a center of rotation and its angle.

The array can be turned into an orthogonal or a circular array by changing its type.</source>
      <translation>Seçilen nesnenin kopyalarını oluşturur ve kopyaları kutupsal bir desene yerleştirir
bir dönme merkezi ve açısı ile tanımlanır.

Dizi, türü değiştirilerek ortogonal veya dairesel bir dizi haline getirilebilir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Polygon</name>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="57"/>
      <source>Polygon</source>
      <translation>Çokgen</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="60"/>
      <source>Creates a regular polygon (triangle, square, pentagon, ...), by defining the number of sides and the circumscribed radius.
CTRL to snap, SHIFT to constrain</source>
      <translation>Kenarların sayısını ve sınırlı yarıçapı tanımlayarak normal bir çokgen (üçgen, kare, beşgen, ...) oluşturur.
Yakalamak için CTRL, sınırlamak için SHIFT</translation>
    </message>
  </context>
  <context>
    <name>Draft_Rectangle</name>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="54"/>
      <source>Rectangle</source>
      <translation>Dikdörtgen</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="57"/>
      <source>Creates a 2-point rectangle. CTRL to snap.</source>
      <translation>2 noktalı bir dikdörtgen oluşturur. Tutturmak için CTRL.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Rotate</name>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="62"/>
      <source>Rotate</source>
      <translation>Döndür</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="65"/>
      <source>Rotates the selected objects. Choose the center of rotation, then the initial angle, and then the final angle.
If the "copy" option is active, it will create rotated copies.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
      <translation>Seçili nesneleri döndürür. Dönme merkezini, ardından ilk açıyı ve ardından son açıyı seçin.
"Kopyala" seçeneği etkinse, döndürülmüş kopyalar oluşturacaktır.
Yakalamak için CTRL, sınırlamak için SHIFT. Her tıklamayla bir kopya oluşturmak için ALT tuşunu basılı tutun ve tıklayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Scale</name>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="71"/>
      <source>Scale</source>
      <translation>Ölçek</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="74"/>
      <source>Scales the selected objects from a base point.
CTRL to snap, SHIFT to constrain, ALT to copy.</source>
      <translation>Temel bir noktaya göre seçilen nesneleri ölçekler. Nokta Yakalamak için CTRL, kısıtlamak için SHIFT, kopyalamak için ALT.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SelectGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="164"/>
      <source>Select group</source>
      <translation>Grubu seç</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="167"/>
      <source>Selects the contents of selected groups. For selected non-group objects, the contents of the group they are in is selected.</source>
      <translation>Seçilen grupların içeriklerini seçer. Seçilen grup dışı nesneler için, içinde bulundukları grup içeriği seçilir.</translation>
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
      <translation>Draft nesnelerinin çizileceği bir çalışma düzlemi oluşturmak için katı gövdenin yüzünü seçin.
Ayrıca üç köşe veya bir Çalışma Düzlemi Proxy'si de seçebilirsiniz.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SetStyle</name>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="55"/>
      <source>Set style</source>
      <translation>Biçim ayarla</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="57"/>
      <source>Sets default styles</source>
      <translation>Varsayılan biçimleri ayarlar</translation>
    </message>
  </context>
  <context>
    <name>Draft_Shape2DView</name>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="59"/>
      <source>Shape 2D view</source>
      <translation>Şekil 2B görünümü</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="62"/>
      <source>Creates a 2D projection of the selected objects on the XY plane.
The initial projection direction is the negative of the current active view direction.
You can select individual faces to project, or the entire solid, and also include hidden lines.
These projections can be used to create technical drawings with the TechDraw Workbench.</source>
      <translation>XY düzleminde seçilen nesnelerin 2B projeksiyonunu oluşturur.
İlk yansıtma yönü, mevcut aktif görünüm yönünün negatifidir.
Yansıtmak için tek tek yüzleri veya katının tamamını seçebilir ve ayrıca gizli çizgiler ekleyebilirsiniz.
Bu projeksiyonlar TechDraw Workbench ile teknik çizimler oluşturmak için kullanılabilir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ShapeString</name>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="66"/>
      <source>Shape from text</source>
      <translation>Metinden şekil</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="68"/>
      <source>Creates a shape from a text string by choosing a specific font and a placement.
The closed shapes can be used for extrusions and boolean operations.</source>
      <translation>Belirli bir yazı tipi ve bir yerleşim seçerek bir metin dizesinden bir şekil oluşturur.
Kapalı şekiller ekstrusyonlar ve mantıksal işlemler için kullanılabilir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ShowSnapBar</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="589"/>
      <source>Show snap toolbar</source>
      <translation>Yakalama Araç çubuğunda göster</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="592"/>
      <source>Show the snap toolbar if it is hidden.</source>
      <translation>Gizli ise, yakalama araç çubuğunu göster.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Slope</name>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="70"/>
      <source>Set slope</source>
      <translation>Eğimi Ayarlayın</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="73"/>
      <source>Sets the slope of the selected line by changing the value of the Z value of one of its points.
If a polyline is selected, it will apply the slope transformation to each of its segments.

The slope will always change the Z value, therefore this command only works well for
straight Draft lines that are drawn in the XY plane. Selected objects that aren't single lines will be ignored.</source>
      <translation>Noktalarından birinin Z değerinin değerini değiştirerek seçilen çizginin eğimini ayarlar.
Bir çoklu çizgi seçilirse, eğim dönüşümünü segmentlerinin her birine uygulayacaktır.

Eğim her zaman Z değerini değiştirir, bu nedenle bu komut yalnızca
XY düzleminde çizilen düz Draft çizgileri. Tek satır olmayan seçili nesneler göz ardı edilecektir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Angle</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="344"/>
      <source>Angle</source>
      <translation>Açı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="347"/>
      <source>Set snapping to points in a circular arc located at multiples of 30 and 45 degree angles.</source>
      <translation>Dairesel bir yayın 30 ve 45 derecelik açı katlarında bulunan noktaları yakalamayı ayarlayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Center</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="374"/>
      <source>Center</source>
      <translation>Ortala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="377"/>
      <source>Set snapping to the center of a circular arc.</source>
      <translation>Dairesel bir yayın merkezini yakalamayı ayarlayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Dimensions</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="526"/>
      <source>Show dimensions</source>
      <translation>Ölçüleri göster</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="529"/>
      <source>Show temporary linear dimensions when editing an object and using other snapping methods.</source>
      <translation>Bir nesneyi düzenlerken ve diğer yakalama yöntemlerini kullanırken geçici doğrusal boyutları gösterin.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Endpoint</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="313"/>
      <source>Endpoint</source>
      <translation>Bitiş noktası</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="316"/>
      <source>Set snapping to endpoints of an edge.</source>
      <translation>Bir kenarın uç noktalarına yakalamayı ayarlayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Extension</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="404"/>
      <source>Extension</source>
      <translation>Uzantı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="407"/>
      <source>Set snapping to the extension of an edge.</source>
      <translation>Bir kenarın uzantısını yakalamayı ayarlayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Grid</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="223"/>
      <source>Grid</source>
      <translation>Izgara</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="226"/>
      <source>Set snapping to the intersection of grid lines.</source>
      <translation>Izgara çizgilerinin kesişme noktalarını yakalama.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Intersection</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="253"/>
      <source>Intersection</source>
      <translation>Kesişim</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="256"/>
      <source>Set snapping to the intersection of edges.</source>
      <translation>Yakalamayı kenarların kesişim noktasına ayarlayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Lock</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="133"/>
      <source>Main snapping toggle On/Off</source>
      <translation>Ana yakalamayı geçişi olarak Aç / Kapa</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="136"/>
      <source>Activates or deactivates all snap methods at once.</source>
      <translation>Tüm yakalama yöntemlerini aynı anda etkinleştirir veya devre dışı bırakır.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Midpoint</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="163"/>
      <source>Midpoint</source>
      <translation>Orta nokta</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="166"/>
      <source>Set snapping to the midpoint of an edge.</source>
      <translation>Bir kenarın orta noktasına yakalamayı ayarlayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Near</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="434"/>
      <source>Nearest</source>
      <translation>Yakınında</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="437"/>
      <source>Set snapping to the nearest point of an edge.</source>
      <translation>Bir kenarın en yakın noktasını yakalamayı ayarlayın.</translation>
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
      <translation>Bir noktadan 45 derecenin katları olan bir yönelim ile yakalamayı ayarlayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Parallel</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="283"/>
      <source>Parallel</source>
      <translation>Koşut</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="286"/>
      <source>Set snapping to a direction that is parallel to an edge.</source>
      <translation>Yakalamayı bir kenara paralel bir yöne ayarlayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Perpendicular</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="193"/>
      <source>Perpendicular</source>
      <translation>Dik</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="196"/>
      <source>Set snapping to a direction that is perpendicular to an edge.</source>
      <translation>Yakalamayı bir kenara dik olan bir yöne ayarlayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Special</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="495"/>
      <source>Special</source>
      <translation>Özel</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="498"/>
      <source>Set snapping to the special points defined inside an object.</source>
      <translation>Bir nesnenin içinde tanımlanan özel noktalara yakalamayı ayarlayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_WorkingPlane</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="559"/>
      <source>Working plane</source>
      <translation>Çalışma düzlemi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="562"/>
      <source>Restricts snapping to a point in the current working plane.
If you select a point outside the working plane, for example, by using other snapping methods,
it will snap to that point's projection in the current working plane.</source>
      <translation>Geçerli çalışma düzleminde bir noktaya yakalamayı kısıtlar.
Çalışma düzleminin dışında bir nokta seçerseniz, örneğin diğer yakalama yöntemlerini kullanarak,
o noktanın mevcut çalışma düzlemindeki izdüşümüne yapışacaktır.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Split</name>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="56"/>
      <source>Split</source>
      <translation>Ayır</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="59"/>
      <source>Splits the selected line or polyline into two independent lines
or polylines by clicking anywhere along the original object.
It works best when choosing a point on a straight segment and not a corner vertex.</source>
      <translation>Seçili çizgiyi veya çoklu çizgiyi iki bağımsız çizgiye böler
veya orijinal nesne boyunca herhangi bir yere tıklayarak çoklu çizgiler.
En iyi sonucu köşe tepe noktası yerine düz bir parça üzerinde bir nokta seçerken işe yarar.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Stretch</name>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="64"/>
      <source>Stretch</source>
      <translation>Uzat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="67"/>
      <source>Stretches the selected objects.
Select an object, then draw a rectangle to pick the vertices that will be stretched,
then draw a line to specify the distance and direction of stretching.</source>
      <translation>Seçili nesneleri uzatır.
Bir nesne seçin, ardından uzatılacak köşeleri seçmek için bir dikdörtgen çizin, 
ardından uzatma mesafesini ve yönünü belirtmek için bir çizgi çizin.</translation>
    </message>
  </context>
  <context>
    <name>Draft_SubelementHighlight</name>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="61"/>
      <source>Subelement highlight</source>
      <translation>Alt eleman vurgusu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="64"/>
      <source>Highlight the subelements of the selected objects, so that they can then be edited with the move, rotate, and scale tools.</source>
      <translation>Daha sonra taşıma, döndürme ve ölçekleme araçlarıyla düzenlenebilmeleri için seçili nesnelerin alt öğelerini vurgulayın.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Text</name>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="60"/>
      <source>Text</source>
      <translation>Metin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="63"/>
      <source>Creates a multi-line annotation. CTRL to snap.</source>
      <translation>Çok satırlı bir açıklama oluşturur. Nokta yakalamak için CTRL.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleConstructionMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="99"/>
      <source>Toggle construction mode</source>
      <translation>İnşa Modunu Değiştir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="102"/>
      <source>Toggles the Construction mode.
When this is active, the following objects created will be included in the construction group, and will be drawn with the specified color and properties.</source>
      <translation>Yapı modunu değiştirir.
Bu aktif olduğunda, oluşturulan aşağıdaki nesneler yapı grubuna dahil edilecek ve belirtilen renk ve özelliklerle çizilecektir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleContinueMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="131"/>
      <source>Toggle continue mode</source>
      <translation>Devam modunu değiştir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="133"/>
      <source>Toggles the Continue mode.
When this is active, any drawing tool that is terminated will automatically start again.
This can be used to draw several objects one after the other in succession.</source>
      <translation>Devam modunu değiştirir.
Bu aktif olduğunda, sonlandırılan herhangi bir çizim aracı otomatik olarak yeniden başlayacaktır.
Bu, birkaç nesneyi birbiri ardına çizmek için kullanılabilir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleDisplayMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="166"/>
      <source>Toggle normal/wireframe display</source>
      <translation>Normal / tel kafes görüntüsünü değiştir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="168"/>
      <source>Switches the display mode of selected objects from flatlines to wireframe and back.
This is helpful to quickly visualize objects that are hidden by other objects.
This is intended to be used with closed shapes and solids, and doesn't affect open wires.</source>
      <translation>Seçili nesnelerin görüntüleme modunu düz çizgilerden tel kafeslere değiştirir ve geri alır.
Bu, diğer nesneler tarafından gizlenen nesneleri hızlı bir şekilde görselleştirmek için yararlıdır.
Bu, kapalı şekiller ve katılarla kullanılmak üzere tasarlanmıştır ve açık telleri etkilemez.</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleGrid</name>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="58"/>
      <source>Toggle grid</source>
      <translation>Izgarayı Aç/Kapa</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="59"/>
      <source>Toggles the Draft grid on and off.</source>
      <translation>Taslak ızgarasını aç ve kapat.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Trimex</name>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="76"/>
      <source>Trimex</source>
      <translation>KırpUzat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="82"/>
      <source>Trims or extends the selected object, or extrudes single faces.
CTRL snaps, SHIFT constrains to current segment or to normal, ALT inverts.</source>
      <translation>Seçilen nesneyi kırpar veya uzatır ya da tek yüzeyleri katılar.
CTRL yakalama, SHIFT geçerli bölüme veya normale sınırlar, ALT tersine çevirir.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Upgrade</name>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="58"/>
      <source>Upgrade</source>
      <translation>Yükselt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="61"/>
      <source>Upgrades the selected objects into more complex shapes.
The result of the operation depends on the types of objects, which may be able to be upgraded several times in a row.
For example, it can join the selected objects into one, convert simple edges into parametric polylines,
convert closed edges into filled faces and parametric polygons, and merge faces into a single face.</source>
      <translation>Seçilen nesneleri daha karmaşık şekillere yükseltir.
İşlemin sonucu, arka arkaya birkaç kez yükseltilebilen nesnelerin türlerine bağlıdır.
Örneğin: Seçilen nesneleri tek bir nesnede birleştirebilir ve basit kenarları parametrik çoklu çizgilere dönüştürebilir,
kapalı kenarları dolgulu yüzlere ve parametrik çokgenlere dönüştürün ve yüzleri tek bir yüze birleştirin.</translation>
    </message>
  </context>
  <context>
    <name>Draft_Wire</name>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="306"/>
      <source>Polyline</source>
      <translation>Çoklu çizgi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="309"/>
      <source>Creates a multiple-points line (polyline). CTRL to snap, SHIFT to constrain.</source>
      <translation>Bir çok-noktalı çizgi oluşturur. Nokta Yakalamak için CTRL, kısıtlamak için SHIFT.</translation>
    </message>
  </context>
  <context>
    <name>Draft_WireToBSpline</name>
    <message>
      <location filename="../../draftguitools/gui_wire2spline.py" line="64"/>
      <source>Wire to B-spline</source>
      <translation>Telden B-spline oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_wire2spline.py" line="67"/>
      <source>Converts a selected polyline to a B-spline, or a B-spline to a polyline.</source>
      <translation>Seçili bir çoklu çizgiyi bir B-spline'a veya bir B-spline'ı bir çoklu çizgiye dönüştürür.</translation>
    </message>
  </context>
  <context>
    <name>Draft_WorkingPlaneProxy</name>
    <message>
      <location filename="../../draftguitools/gui_planeproxy.py" line="50"/>
      <source>Create working plane proxy</source>
      <translation>Çalışma Düzlemi Proxy'si Oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_planeproxy.py" line="52"/>
      <source>Creates a proxy object from the current working plane.
Once the object is created double click it in the tree view to restore the camera position and objects' visibilities.
Then you can use it to save a different camera position and objects' states any time you need.</source>
      <translation>Mevcut çalışma düzleminden bir proxy (vekil) nesnesi oluşturur.
Nesne oluşturulduktan sonra, kamera konumunu ve nesnelerin görünürlüğünü geri yüklemek için ağaç görünümünde nesneye çift tıklayın.
Ardından, ihtiyacınız olan her an farklı bir kamera konumunu ve nesnelerin durumlarını kaydetmek için kullanabilirsiniz.</translation>
    </message>
  </context>
  <context>
    <name>Form</name>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="14"/>
      <source>Working plane setup</source>
      <translation>Çalışma düzlemi kurulumu</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="20"/>
      <source>Select a face or working plane proxy or 3 vertices.
Or choose one of the options below</source>
      <translation>Select a face or working plane proxy or 3 vertices.
Or choose one of the options below</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="31"/>
      <source>Sets the working plane to the XY plane (ground plane)</source>
      <translation>Çalışma düzlemini XY düzlemine ayarla (zemin düzlemi)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="34"/>
      <source>Top (XY)</source>
      <translation>Üst (XY)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="41"/>
      <source>Sets the working plane to the XZ plane (front plane)</source>
      <translation>Çalışma düzlemini XZ düzlemine ayarla (ön düzlem)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="44"/>
      <source>Front (XZ)</source>
      <translation>Ön (XZ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="51"/>
      <source>Sets the working plane to the YZ plane (side plane)</source>
      <translation>Çalışma düzlemini YZ düzlemine ayarla (yan düzlem)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="54"/>
      <source>Side (YZ)</source>
      <translation>Yan (YZ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="61"/>
      <source>Sets the working plane facing the current view</source>
      <translation>Çalışma düzlemini mevcut görünüme göre ayarla</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="64"/>
      <source>Align to view</source>
      <translation>Görünüme Hizala</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="71"/>
      <source>The working plane will align to the current
view each time a command is started</source>
      <translation>The working plane will align to the current
view each time a command is started</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="78"/>
      <source>Automatic</source>
      <translation>Otomatik</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="87"/>
      <source>Offset</source>
      <translation>Uzaklaşma</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="94"/>
      <source>An optional offset to give to the working plane
above its base position. Use this together with one
of the buttons above</source>
      <translation>An optional offset to give to the working plane
above its base position. Use this together with one
of the buttons above</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="106"/>
      <location filename="../ui/TaskSelectPlane.ui" line="118"/>
      <source>If this is selected, the working plane will be
centered on the current view when pressing one
of the buttons above</source>
      <translation>If this is selected, the working plane will be
centered on the current view when pressing one
of the buttons above</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="111"/>
      <source>Center plane on view</source>
      <translation>Görünümde düzlem merkezi</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="135"/>
      <source>Or select a single vertex to move the current
working plane without changing its orientation.
Then, press the button below</source>
      <translation>Veya geçerli çalışma düzlemini, yönelimini 
değiştirmeden taşımak için tek bir köşe seçin.
Sonra aşağıdaki düğmeye basın</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="147"/>
      <source>Moves the working plane without changing its
orientation. If no point is selected, the plane
will be moved to the center of the view</source>
      <translation>Çalışma düzlemini, yönelimini değiştirmeden taşır.
Eğer bir nokta seçilmezse düzlem, görünüm 
merkezine taşınacaktır</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="152"/>
      <source>Move working plane</source>
      <translation>Çalışma düzlemini taşı</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="161"/>
      <location filename="../ui/TaskSelectPlane.ui" line="171"/>
      <source>The spacing between the smaller grid lines</source>
      <translation>Her bir kılavuz çizgisi arasındaki mesafe</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="164"/>
      <source>Grid spacing</source>
      <translation>Izgara aralığı</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="181"/>
      <location filename="../ui/TaskSelectPlane.ui" line="191"/>
      <source>The number of squares between each main line of the grid</source>
      <translation>Izgaranın her ana çizgisi arasındaki kare sayısı</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="184"/>
      <source>Main line every</source>
      <translation>Ana çizgilerin tümü</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="198"/>
      <source>Grid extension</source>
      <translation>Izgara uzantısı</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="205"/>
      <source> lines</source>
      <translation> çizgiler</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="218"/>
      <location filename="../ui/TaskSelectPlane.ui" line="230"/>
      <source>The distance at which a point can be snapped to
when approaching the mouse. You can also change this
value by using the [ and ] keys while drawing</source>
      <translation>Fare yaklaştırıldığında yakalanabilecek bir noktanın 
mesafesi. Bu değeri [ ve ] tuşlarını kullanarak çizim 
sırasında değiştirebilirsiniz</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="223"/>
      <source>Snapping radius</source>
      <translation>Yakalama yarıçapı (etki alanı)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="241"/>
      <source>Centers the view on the current working plane</source>
      <translation>Mevcut çalışma düzlemindeki görünümü merkeze alır</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="244"/>
      <source>Center view</source>
      <translation>Merkezî görünüm</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="251"/>
      <source>Resets the working plane to its previous position</source>
      <translation>Çalışma düzlemini önceki konumuna sıfırlar</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="254"/>
      <source>Previous</source>
      <translation>Önceki</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="14"/>
      <source>Style settings</source>
      <translation>Biçim Ayarları</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="22"/>
      <source>Fills the values below with a stored style preset</source>
      <translation>Aşağıdaki değerleri kayıtlı bir biçim hazır ayarı ile doldurur</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="26"/>
      <source>Load preset</source>
      <translation>Hazır ayar yükle</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="40"/>
      <source>Save current style as a preset...</source>
      <translation>Geçerli biçimi hazır ayar olarak kaydet...</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="57"/>
      <source>Lines and faces</source>
      <translation>Çizgiler ve yüzeyler</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="66"/>
      <source>Line color</source>
      <translation>Çizgi rengi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="73"/>
      <source>The color of lines</source>
      <translation>Çizgi rengi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="80"/>
      <source>Line width</source>
      <translation>Çizgi Kalınlığı</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="87"/>
      <source> px</source>
      <translation> piksel</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="94"/>
      <source>Draw style</source>
      <translation>Çizim biçimi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="101"/>
      <source>The line style</source>
      <translation>Çizgi biçimi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="105"/>
      <source>Solid</source>
      <translation>Katı</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="110"/>
      <source>Dashed</source>
      <translation>Kesik çizgili</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="115"/>
      <source>Dotted</source>
      <translation>Noktalı</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="120"/>
      <source>DashDot</source>
      <translation>Çizgi nokta</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="128"/>
      <source>Display mode</source>
      <translation>Ekran kipi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="135"/>
      <source>The display mode for faces</source>
      <translation>Yüzler için görüntüleme modu</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="139"/>
      <source>Flat Lines</source>
      <translation>Düz çizgiler</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="144"/>
      <source>Wireframe</source>
      <translation>Tel Kafes görünümü</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="149"/>
      <source>Shaded</source>
      <translation>Gölgeli</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="154"/>
      <source>Points</source>
      <translation>Noktalar</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="162"/>
      <source>Shape color</source>
      <translation>Şekil rengi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="169"/>
      <source>The color of faces</source>
      <translation>Yüzey renkleri</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="176"/>
      <source>Transparency</source>
      <translation>Şeffaflık</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="183"/>
      <source>The transparency of faces</source>
      <translation>Yüzlerin şeffaflığı</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="186"/>
      <source> %</source>
      <translation> %</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="199"/>
      <source>Annotations</source>
      <translation>Açıklamalar</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="205"/>
      <source>Text font</source>
      <translation>Metin Yazı Tipi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="218"/>
      <source>The font to use for texts and dimensions</source>
      <translation>Metinler ve boyutlar için kullanılacak yazı tipi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="225"/>
      <source>Text size</source>
      <translation>Metin boyutu</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="232"/>
      <source>The size of texts and dimension texts</source>
      <translation>Metin ve ölçü metinleri boyutu</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="242"/>
      <source>Text spacing</source>
      <translation>Birim Aralığı</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="249"/>
      <source>The space between the text and the dimension line</source>
      <translation>Metin ve ölçü çizgisi arasındaki boşluk</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="259"/>
      <source>Text color</source>
      <translation>Metin rengi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="266"/>
      <source>The color of texts and dimension texts</source>
      <translation>Metin ve ölçü metinleri rengi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="273"/>
      <source>Line spacing</source>
      <translation>Satır aralığı</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="280"/>
      <source>The spacing between different lines of text</source>
      <translation>Metnin farklı satırları arasındaki boşluk</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="287"/>
      <source>Arrow style</source>
      <translation>Ok biçimi</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="294"/>
      <source>The type of dimension arrows</source>
      <translation>Ölçü okları türü</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="298"/>
      <source>Dot</source>
      <translation>Nokta</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="303"/>
      <source>Circle</source>
      <translation>Daire</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="308"/>
      <source>Arrow</source>
      <translation>Ok İşareti</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="313"/>
      <source>Tick</source>
      <translation>işaretleme</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="318"/>
      <source>Tick-2</source>
      <translation>Tik-2</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="326"/>
      <source>Arrow size</source>
      <translation>Ok boyu</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="333"/>
      <source>The size of dimension arrows</source>
      <translation>Ölçü okları boyutu</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="343"/>
      <source>Show unit</source>
      <translation>Birimi göster</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="350"/>
      <source>If the unit suffix is shown on dimension texts or not</source>
      <translation>Ölçlü yazılarında ölçü birimi gösterilsin veya gösterilmesin</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="363"/>
      <source>Unit override</source>
      <translation>Birim geçersiz kılma</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="370"/>
      <source>The unit to use for dimensions. Leave blank to use current FreeCAD unit</source>
      <translation>Ölçülendirme için kullanılacak ölçü birimi. Mevcut FreeCAD ölçü birimini kullanmak için boş bırakın</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="382"/>
      <source>Apply above style to selected object(s)</source>
      <translation>Biçimi, seçilen nesne(ler) üzerine uygula</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="385"/>
      <source>Selected</source>
      <translation>Seçili</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="397"/>
      <source>Texts/dims</source>
      <translation>Metinler/ölçüler</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="14"/>
      <source>Form</source>
      <translation>Şekil:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="20"/>
      <source>PAT file:</source>
      <translation>PAT dosyası:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="27"/>
      <source>pattern files (*.pat)</source>
      <translation>desen dosyaları (*.pat)</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="34"/>
      <source>Pattern:</source>
      <translation>Desen:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="44"/>
      <source>Scale</source>
      <translation>Ölçek</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="64"/>
      <source>Rotation:</source>
      <translation>Döndürme:</translation>
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
      <translation>Genel ayarlar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="32"/>
      <source>General Draft Settings</source>
      <translation>Genel Taslak Ayarları</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="40"/>
      <source>Default working plane</source>
      <translation>Varsayılan çalışma düzlemi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="67"/>
      <source>None</source>
      <translation>Hiçbiri</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="72"/>
      <source>XY (Top)</source>
      <translation>XY (Üst)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="77"/>
      <source>XZ (Front)</source>
      <translation>XZ (Ön)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="82"/>
      <source>YZ (Side)</source>
      <translation>YZ (yan)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="94"/>
      <source>Internal precision level</source>
      <translation>Dahili duyarlık düzeyi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="120"/>
      <source>The number of decimals in internal coordinates operations (for ex. 3 = 0.001). Values between 6 and 8 are usually considered the best trade-off among FreeCAD users.</source>
      <translation>Dahili koordinat işlemlerinde ondalık sayısı (örneğin, 3 = 0,001). 6 ile 8 arasındaki değerler genellikle FreeCAD kullanıcıları arasında en iyi takas sayılır.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="143"/>
      <source>Tolerance</source>
      <translation>Tolerans</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="169"/>
      <source>This is the value used by functions that use a tolerance.
Values with differences below this value will be treated as same. This value will be obsoleted soon so the precision level above controls both.</source>
      <translation>Tolerans kullanan fonksiyonların kullandığı değer budur. 
Bu değerin altında farklılık gösteren değerler aynı şekilde ele alınacaktır. Bu değer yakında kullanılmayacak, böylece yukarıdaki hassasiyet seviyesi her ikisini de kontrol edebilecek.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="194"/>
      <source>If this option is checked, the layers drop-down list will also show groups, allowing you to automatically add objects to groups too.</source>
      <translation>Bu seçenek işaretlenirse; açılır katman listesi, grupları da gösterecek ayrıca gruplara otomatik olarak nesne eklemenize de izin verecektir.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="197"/>
      <source>Show groups in layers list drop-down button</source>
      <translation>Açılır katman listesi düğmesinde grupları göster</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="216"/>
      <source>Draft tools options</source>
      <translation>Taslak araçları seçenekleri</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="230"/>
      <source>When drawing lines, set focus on Length instead of X coordinate.
This allows to point the direction and type the distance.</source>
      <translation>Çizgiler çizerken odağı, X koordinatı yerine Uzunluğa ayarla.
Bu, yönü çevirmenize ve mesafeyi girmenize izin verir.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="234"/>
      <source>Set focus on Length instead of X coordinate</source>
      <translation>Odağı X koordinatı yerine Uzunluğa ayarla</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="247"/>
      <source>Normally, after copying objects, the copies get selected.
If this option is checked, the base objects will be selected instead.</source>
      <translation>Normalde, nesneleri kopyaladıktan sonra kopyalar seçili kalır. 
Bu seçenek işaretli ise, onun yerine temel nesneler seçili olacaktır.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="251"/>
      <source>Select base objects after copying</source>
      <translation>Kopyaladıktan sonra temel nesneleri seç</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="264"/>
      <source>If this option is set, when creating Draft objects on top of an existing face of another object, the "Support" property of the Draft object will be set to the base object. This was the standard behaviour before FreeCAD 0.19</source>
      <translation>Bu seçenek ayarlanmışsa, başka bir nesnenin varolan yüzeyinin üstüne Taslak nesneler oluştururken, Taslak nesnesinin "Destek" özelliği temel nesneye ayarlanır. Bu, FreeCAD 0.19'dan önceki standart davranıştır</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="267"/>
      <source>Set the Support property when possible</source>
      <translation>Mümkün olduğunda Destek özelliğini ayarla</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="280"/>
      <source>If this is checked, objects will appear as filled by default.
Otherwise, they will appear as wireframe</source>
      <translation>Bu işaretlenirse, nesneler varsayılan olarak içi dolu görünecektir. 
Aksi takdirde, tel kafes olarak görünecektir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="284"/>
      <source>Fill objects with faces whenever possible</source>
      <translation>Nesneleri ile karşı karşıya mümkün olduğunca doldurmak</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="300"/>
      <source>If this is checked, copy mode will be kept across command,
otherwise commands will always start in no-copy mode</source>
      <translation>Bu işaretlenirse, kopyalama modu komutlar arasında tutulacaktır,
aksi takdirde komutlar her zaman kopyasız modda başlar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="304"/>
      <source>Global copy mode</source>
      <translation>Genel kopyalama kipi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="320"/>
      <source>Force Draft Tools to create Part primitives instead of Draft objects.
Note that this is not fully supported, and many object will be not editable with Draft Modifiers.</source>
      <translation>Draft Araçlarını, Draft nesneleri yerine Parça temel öğeleri oluşturmaya zorlayın.
Bunun tam olarak desteklenmediğini ve birçok nesnenin Taslak Değiştiricilerle düzenlenemeyeceğini unutmayın.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="324"/>
      <source>Use Part Primitives when available</source>
      <translation>Kısım temel öğeler kullanılabilir olduğunda kullanın</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="341"/>
      <source>Prefix labels of Clones with:</source>
      <translation>Klon ile önek etiketleri:</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="372"/>
      <source>Construction Geometry</source>
      <translation>Yapı Geometrisi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="380"/>
      <source>Construction group name</source>
      <translation>Yapı grup adı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="387"/>
      <source>This is the default group name for construction geometry</source>
      <translation>Bu, inşa geometrisi için varsayılan grup adıdır.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="390"/>
      <source>Construction</source>
      <translation>İnşa</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="407"/>
      <source>Construction geometry color</source>
      <translation>Yapı geometrisi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="427"/>
      <source>This is the default color for objects being drawn while in construction mode.</source>
      <translation>Bu, inşa kipinde çizine nesneler için varsayılan renktir.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="14"/>
      <source>Visual settings</source>
      <translation>Görsel ayarlar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="26"/>
      <source>Visual Settings</source>
      <translation>Görsel ayarlar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="34"/>
      <source>Snap symbols style</source>
      <translation>Nokta Yakalama Simgeleri Biçimi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="54"/>
      <source>Draft classic style</source>
      <translation>Taslak klasik stili</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="59"/>
      <source>Bitsnpieces style</source>
      <translation>Bitsnpieces tarzı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="80"/>
      <source>Color</source>
      <translation>Renk</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="87"/>
      <source>The default color for snap symbols</source>
      <translation>Nokta Yakalamak simgeleri için varsayılan renk</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="111"/>
      <source>Check this if you want to use the color/linewidth from the toolbar as default</source>
      <translation>Renk/linewidth araç çubuğu'nda varsayılan olarak kullanmak istiyorsanız bunu işaretleyin</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="114"/>
      <source>Save current color and linewidth across sessions</source>
      <translation>Oturumlar arası kullanım için mevcut rengi ve çizgi kalınlığını sakla</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="131"/>
      <source>If checked, a widget indicating the current working plane orientation appears during drawing operations</source>
      <translation>Bu onay kutusu seçiliyse, çizim işlemleri sırasında geçerli çalışma ortamının yönünü gösteren bir araç gözükür</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="134"/>
      <source>Show Working Plane tracker</source>
      <translation>Çalışma Alanı izleyicisini göster</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="151"/>
      <source>Default template sheet</source>
      <translation>Varsayılan şablon sayfası</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="177"/>
      <source>The default template to use when creating a new drawing sheet</source>
      <translation>Yeni bir çizim sayfası oluştururken kullanılacak varsayılan şablon</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="194"/>
      <source>Alternate SVG patterns location</source>
      <translation>Alternatif SVG desenleri konumu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="220"/>
      <source>Here you can specify a directory with custom SVG files containing &lt;pattern&gt; definitions to be added to the standard patterns</source>
      <translation>desen</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="237"/>
      <source>SVG pattern resolution</source>
      <translation>SVG desen çözünürlüğü</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="257"/>
      <source>The resolution to draw the patterns in. Default value is 128. Higher values give better resolutions, lower values make drawing faster</source>
      <translation>Desenleri çizmek için varsyılan çözünürlük değeri 128 dir. Daha yüksek değerler daha iyi görüntü sağlar. daha düşük değerler ise daha hızlı grafikler sağlar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="280"/>
      <source>SVG pattern default size</source>
      <translation>SVG desen varsayılan boyutu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="300"/>
      <source>The default size for SVG patterns</source>
      <translation>SVG desenler için varsayılan boyut</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="326"/>
      <source>Check this if you want to preserve colors of faces while doing downgrade and upgrade (splitFaces and makeShell only)</source>
      <translation>Sürüm indirme ve yükseltme yaparken yüzeylerin renklerini korumak istiyorsanız bunu işaretleyin (yalnızca splitFaces ve makeShell)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="329"/>
      <source>Preserve colors of faces during downgrade/upgrade</source>
      <translation>Sürüm indirme / yükseltme sırasında yüzlerin renklerini koru</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="346"/>
      <source>Check this if you want the face names to derive from the originating object name and vice versa while doing downgrade/upgrade (splitFaces and makeShell only)</source>
      <translation>Sürüm indirme / yükseltme yaparken yüzey isimlendirmelerin kaynak nesneden esinlenilerek türetilmesini ve tersini istiyorsanız bu seçeneği işaretleyin (yalnızca splitFaces [YüzeyleriAyır] ve makeShell[KabukOluştur])</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="349"/>
      <source>Preserve names of faces during downgrade/upgrade</source>
      <translation>Sürüm indirme / yükseltme sırasında yüzeylerin isimlerini koru</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="367"/>
      <source>Drawing view line definitions</source>
      <translation>Çizim görünümü satır tanımları</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="375"/>
      <source>Dashed line definition</source>
      <translation>Kesik çizgi tanımı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="395"/>
      <location filename="../ui/preferences-draftvisual.ui" line="438"/>
      <location filename="../ui/preferences-draftvisual.ui" line="481"/>
      <source>An SVG linestyle definition</source>
      <translation>Bir SVG linestyle tanımı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="398"/>
      <source>0.09,0.05</source>
      <translation>0.09,0.05</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="418"/>
      <source>Dashdot line definition</source>
      <translation>Kesik çizgi tanımı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="441"/>
      <source>0.09,0.05,0.02,0.05</source>
      <translation>0.09,0.05,0.02,0.05</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="461"/>
      <source>Dotted line definition</source>
      <translation>Kesik çizgi tanımı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="484"/>
      <source>0.02,0.02</source>
      <translation>0.02,0.02</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="14"/>
      <source>Texts and dimensions</source>
      <translation>Metinler ve ölçüler</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="26"/>
      <source>Text settings</source>
      <translation>Metin ayarları</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="34"/>
      <source>Font family</source>
      <translation>Yazı tipi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="47"/>
      <source>This is the default font name for all Draft texts and dimensions.
It can be a font name such as "Arial", a default style such as "sans", "serif"
or "mono", or a family such as "Arial,Helvetica,sans" or a name with a style
such as "Arial:Bold"</source>
      <translation>Bu, mevcut taslak metinler ve boyutlar için varsayılan yazı tipi adıdır.
Bu değer, "Arial", varsayılan stiller "sans", "serif" veya "mono", veya aile adı olarak "Arial,Helvetica,sans", veya stil ile herhangi bir ad "Arial:Bold" gibi yazı tipi değerleri alabilir.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="59"/>
      <source>Internal font</source>
      <translation>İç yazı tipi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="76"/>
      <source>Font size</source>
      <translation>Yazı Boyutu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="89"/>
      <source>Default height for texts and dimensions</source>
      <translation>Metinler ve boyutlar için kullanılacak yazı tipi</translation>
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
      <translation>Boyut ayarları</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="124"/>
      <source>Display mode</source>
      <translation>Ekran kipi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="144"/>
      <source>text above (2D)</source>
      <translation>metin (2D) yukarıda</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="149"/>
      <source> text inside (3D)</source>
      <translation> (3D) içindeki metin</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="161"/>
      <source>Number of decimals</source>
      <translation>Ondalık basamak sayısı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="201"/>
      <source>Extension lines size</source>
      <translation>Uzatma çizgileri boyutu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="208"/>
      <source>The default size of dimensions extension lines</source>
      <translation>Boyutları uzantı çizgileri varsayılan boyutu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="237"/>
      <source>Extension line overshoot</source>
      <translation>Uzatma çizgisi aşımı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="244"/>
      <source>The default length of extension line above dimension line</source>
      <translation>Ölçülendirme çizgisinin üzerindeki varsayılan uzantı hattı mesafesi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="273"/>
      <source>Dimension line overshoot</source>
      <translation>Ölçülendirme çizgisi aşımı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="280"/>
      <source>The default distance the dimension line is extended past extension lines</source>
      <translation>Varsayılan ölçülendirme çizgisi mesafesi, uzantı hatlarını geçecek şekilde genişletilir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="309"/>
      <source>Arrows style</source>
      <translation>Okların biçimi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="323"/>
      <source>Dot</source>
      <translation>Nokta</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="328"/>
      <source>Circle</source>
      <translation>Daire</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="333"/>
      <source>Arrow</source>
      <translation>Ok İşareti</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="338"/>
      <source>Tick</source>
      <translation>işaretleme</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="343"/>
      <source>Tick-2</source>
      <translation>Tik-2</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="355"/>
      <source>Arrows size</source>
      <translation>Okların boyutu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="362"/>
      <source>The default size of arrows</source>
      <translation>Ok İşaretleri varsayılan boyutu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="388"/>
      <source>Text orientation</source>
      <translation>Metin yönlendirmesi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="395"/>
      <source>This is the orientation of the dimension texts when those dimensions are vertical. Default is left, which is the ISO standard.</source>
      <translation>Bu, boyutlar dikey olduğunda boyut yazılarının yönüdür. Varsayılan ISO standardı olarak Soldur.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="405"/>
      <source>Left (ISO standard)</source>
      <translation>Sol (ISO standardı)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="410"/>
      <source>Right</source>
      <translation>Sağ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="422"/>
      <source>Text spacing</source>
      <translation>Birim Aralığı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="429"/>
      <source>The space between the dimension line and the dimension text</source>
      <translation>Ölçü metni ile ölçü çizgisi arasındaki boşluk mesafesi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="455"/>
      <source>Show the unit suffix in dimensions</source>
      <translation>Boyutları içinde birim soneki göster</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="475"/>
      <source>Override unit</source>
      <translation>Geçersiz kılma birimi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="495"/>
      <source>By leaving this field blank, the dimension measurements will be shown in the current unit defined in FreeCAD. By indicating a unit here such as m or cm, you can force new dimensions to be shown in that unit.</source>
      <translation>Bu alanın boş bırakılmasıyla, boyut ölçümleri FreeCAD' teki mevcut tanımlı birimde gösterilecek. Buraya m veya cm olarak bir birim girilmesiyle, yeni boyutların o birimde gösterilmesini zorlayabilirsiniz.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="513"/>
      <source>ShapeString settings</source>
      <translation>ShapeString ayarları</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="521"/>
      <source>Default ShapeString font file</source>
      <translation>Varsayılan ShapeString yazı tipi dosyası</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="534"/>
      <source>Select a font file</source>
      <translation>Grip Dosyasını Seçin</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="14"/>
      <source>SVG</source>
      <translation>SVG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="40"/>
      <source>Import style</source>
      <translation>Stili içeri aktar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="47"/>
      <source>Method chosen for importing SVG object color to FreeCAD</source>
      <translation>SVG nesne rengini FreeCAD' e aktarmak için seçilen yöntem</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="60"/>
      <source>None (fastest)</source>
      <translation>Hiçbiri (en hızlısı)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="65"/>
      <source>Use default color and linewidth</source>
      <translation>Varsayılan renk ve çizgi kalınlığı kullanın.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="70"/>
      <source>Original color and linewidth</source>
      <translation>Öz renk ve çizgi kalınlığını kullan</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="82"/>
      <source>If checked, no units conversion will occur.
One unit in the SVG file will translate as one millimeter. </source>
      <translation>İşaretlenirse, birim dönüşümü gerçekleşmez.
SVG dosyasındaki bir birim, bir milimetre olarak çevrilir. </translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="86"/>
      <source>Disable units scaling</source>
      <translation>Ölçekleme birimleri devre dışı bırakma</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="121"/>
      <source>Export style</source>
      <translation>Dışa Aktarım Biçimi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="128"/>
      <source>Style of SVG file to write when exporting a sketch</source>
      <translation>Bir çizimi dışa aktarırken yazılacak SVG dosyasının biçimi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="141"/>
      <source>Translated (for print &amp; display)</source>
      <translation>Dönüştürüldü (baskı &amp; görüntü için)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="146"/>
      <source>Raw (for CAM)</source>
      <translation>Ham (CAM için)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="158"/>
      <source>All white lines will appear in black in the SVG for better readability against white backgrounds</source>
      <translation>Daha iyi okunabilirlik için beyaz arka planlara karşı SVG'de tüm beyaz çizgiler siyah olarak görünecektir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="161"/>
      <source>Translate white line color to black</source>
      <translation>Beyaz çizgi rengini siyaha çevir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="181"/>
      <source>Max segment length for discretized arcs</source>
      <translation>Max segment uzunluğu discretized yaylar için</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="204"/>
      <source>Versions of Open CASCADE older than version 6.8 don't support arc projection.
In this case arcs will be discretized into small line segments.
This value is the maximum segment length. </source>
      <translation>6.8 sürümünden daha eski Open CASCADE sürümleri ark projeksiyonunu desteklemez.
Bu durumda yaylar küçük çizgi parçalarına ayrılacaktır.
Bu değer, maksimum parça uzunluğudur. </translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="14"/>
      <source>OCA</source>
      <translation>OCA</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="46"/>
      <source>Check this if you want the areas (3D faces) to be imported too.</source>
      <translation>Eğer alanlarında (3B yüzeylerin de) içeri aktarılmasını istiyorsanız, bunu işaretleyin.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="49"/>
      <source>Import OCA areas</source>
      <translation>OCA alanlarını içeri aktar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="14"/>
      <source>DXF</source>
      <translation>DXF</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="35"/>
      <source>This preferences dialog will be shown when importing/ exporting DXF files</source>
      <translation>Bu tercihler iletisi, DXF dosyalarının içe/dışa aktarılması sırasında gösterilecek</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="38"/>
      <source>Show this dialog when importing and exporting</source>
      <translation>İçe ve dışa aktarırken bu iletişim kutusunu göster</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="51"/>
      <source>Python importer is used, otherwise the newer C++ is used.
Note: C++ importer is faster, but is not as featureful yet</source>
      <translation>Python içe aktarıcı kullanılır, aksi takdirde daha yeni C ++ kullanılır.
Not: C ++ içe aktarıcı daha hızlıdır, ancak henüz o kadar özellikli değildir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="55"/>
      <source>Use legacy python importer</source>
      <translation>Eski Python içe aktarıcıyı kullanın</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="71"/>
      <source>Python exporter is used, otherwise the newer C++ is used.
Note: C++ exporter is faster, but is not as featureful yet</source>
      <translation>Python dışa aktarıcı, yoksa daha yeni C ++ kullanılır.
Not: C ++ içe aktarıcı daha hızlıdır, ancak henüz o kadar özellikli değildir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="75"/>
      <source>Use legacy python exporter</source>
      <translation>Eski python dışa aktarımcısını kullanın</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="88"/>
      <source>Automatic update (legacy importer only)</source>
      <translation>Otomatik olarak güncelle (yalnızca içe aktarım kalıntıları)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="96"/>
      <source>Allow FreeCAD to download the Python converter for DXF import and export.
You can also do this manually by installing the "dxf_library" workbench
from the Addon Manager.</source>
      <translation>DXF içe ve dışa aktarma için FreeCAD' in Python dönüştürücüyü indirmesine izin ver.
Bu işlemi Eklenti Yöneticisi' nden "dxf_library" tezgahını yükleyerek elle de yapabilirsiniz.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="101"/>
      <source>Allow FreeCAD to automatically download and update the DXF libraries</source>
      <translation>FreeCAD'in DXF kitaplıklarını otomatik olarak indirmesine ve güncellemesine izin ver</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="26"/>
      <location filename="../ui/preferences-dxf.ui" line="119"/>
      <location filename="../ui/preferences-svg.ui" line="26"/>
      <source>Import options</source>
      <translation>İçe aktarım seçenekleri</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="140"/>
      <source>Note: Not all the options below are used by the new importer yet</source>
      <translation>Not: Aşağıdaki tüm seçenekler henüz yeni içe aktarıcı tarafından kullanılmaz</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="149"/>
      <source>Import</source>
      <translation>İçe aktar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="156"/>
      <source>If unchecked, texts and mtexts won't be imported</source>
      <translation>İşaretli değilse, metinler ve çoklu metinler içe aktarılmayacaktır</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="159"/>
      <source>texts and dimensions</source>
      <translation>metinler ve ölçüler</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="172"/>
      <source>If unchecked, points won't be imported</source>
      <translation>İşaretli değilse noktalar içe aktarılmayacak</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="175"/>
      <source>points</source>
      <translation>noktalar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="188"/>
      <source>If checked, paper space objects will be imported too</source>
      <translation>Bu işaretlenirse, kağıt alanı nesneleri de içe aktarılacaktır</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="191"/>
      <source>layouts</source>
      <translation>görünümler</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="204"/>
      <source>If you want the non-named blocks (beginning with a *) to be imported too</source>
      <translation>Adsız blokların (* ile başlayan) da içe aktarılmasını istiyorsanız</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="207"/>
      <source>*blocks</source>
      <translation>*kalınlık ver</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="224"/>
      <source>Create</source>
      <translation>Oluştur</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="231"/>
      <source>Only standard Part objects will be created (fastest)</source>
      <translation>Sadece standart Parça nesneleri oluşturulacak (en hızlısı)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="234"/>
      <source>simple Part shapes</source>
      <translation>basit bölüm şekiller</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="250"/>
      <source>Parametric Draft objects will be created whenever possible</source>
      <translation>Parametrik Taslak nesneleri mümkün olduğunda oluşturulacaktır</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="253"/>
      <source>Draft objects</source>
      <translation>Taslak nesneleri</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="266"/>
      <source>Sketches will be created whenever possible</source>
      <translation>Mümkün olduğunca eskizler oluşturulacak</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="269"/>
      <source>Sketches</source>
      <translation>Eskizler</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="289"/>
      <source>Scale factor to apply to imported files</source>
      <translation>İçe aktarılan dosyalara uygulamak için ölçek faktörü</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="309"/>
      <source>Scale factor to apply to DXF files on import.
The factor is the conversion between the unit of your DXF file and millimeters.
Example: for files in millimeters: 1, in centimeters: 10,
                             in meters: 1000, in inches: 25.4, in feet: 304.8</source>
      <translation>İçe aktarma sırasında ölçek etmenini DXF dosyalarına uygular.
Bu etmen, DXF dosyanızın uzunluk birimi ile milimetre arasındaki dönüşümdür.
Örneğin; milimetreli dosyalar için: 1, santimetre için: 10,
metre için: 1000, inç için: 25.4, feet için: 304.8</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="338"/>
      <source>Colors will be retrieved from the DXF objects whenever possible.
Otherwise default colors will be applied. </source>
      <translation>Renkler, mümkün olduğunda DXF nesnelerinden alınacaktır.
Aksi halde varsayılan renkler uygulanacaktır. </translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="342"/>
      <source>Get original colors from the DXF file</source>
      <translation>Orijinal renkleri DXF dosya olsun</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="359"/>
      <source>FreeCAD will try to join coincident objects into wires.
Note that this can take a while!</source>
      <translation>FreeCAD, çakışan nesneleri tel halinde birleştirmeye çalışacaktır.
Bunun biraz zaman alabileceğini unutmayın!</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="363"/>
      <source>Join geometry</source>
      <translation>Geometriyi birleştir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="380"/>
      <source>Objects from the same layers will be joined into Draft Blocks,
turning the display faster, but making them less easily editable </source>
      <translation>Aynı katmanlardaki nesneler Draft Blocks'ta birleştirilerek ekranı daha hızlı hale getirecek,
ancak onları daha kolay düzenlenemez hale getirecek </translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="384"/>
      <source>Group layers into blocks</source>
      <translation>Grup katmanları bloklara</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="401"/>
      <source>Imported texts will get the standard Draft Text size,
instead of the size they have in the DXF document</source>
      <translation>İçe aktarılan metinler, DXF belgesinde sahip oldukları boyut yerine 
standart Taslak Metin boyutunu alır</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="405"/>
      <source>Use standard font size for texts</source>
      <translation>Metinler için standart yazı tipi boyutunu kullan</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="422"/>
      <source>If this is checked, DXF layers will be imported as Draft Layers</source>
      <translation>Bu işaretlenirse, DXF katmanları Taslak Katmanlar olarak içe aktarılacaktır</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="425"/>
      <source>Use Layers</source>
      <translation>Katmanları kullan</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="445"/>
      <source>Hatches will be converted into simple wires</source>
      <translation>Taramalar basit tellere dönüştürülecek</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="448"/>
      <source>Import hatch boundaries as wires</source>
      <translation>Tarama sınırlarını tel olarak içe aktar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="465"/>
      <source>If polylines have a width defined, they will be rendered
as closed wires with correct width</source>
      <translation>Çoklu çizgilerin kalınlığı tanımlanmışsa, 
bunlar doğru kalınlığa sahip kapalı teller olarak işlenecektir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="469"/>
      <source>Render polylines with width</source>
      <translation>ÇokluÇizgileri kalınlıklı olarak işle (Render'la)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="486"/>
      <source>Ellipse export is poorly supported. Use this to export them as polylines instead.</source>
      <translation>Elips dışa aktarımı desteği yetersizdir. Bunun yerine bunları çoklu çizgi olarak dışa aktarmak için bunu kullanın.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="489"/>
      <source>Treat ellipses and splines as polylines</source>
      <translation>Elipsler ve Spline bağlantılı çizgiler tedavi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="518"/>
      <source>Max Spline Segment:</source>
      <translation>Maksimum düzlem dilimi:</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="528"/>
      <source>Maximum length of each of the polyline segments.
If it is set to '0' the whole spline is treated as a straight segment.</source>
      <translation>Her çoklu çizginin her bir parçasının en büyük uzunluğu, eğer değeri 0 olarak atanır ise her parça düz olur.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="559"/>
      <location filename="../ui/preferences-svg.ui" line="107"/>
      <source>Export options</source>
      <translation>Dışa aktarım seçenekleri</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="567"/>
      <source>All objects containing faces will be exported as 3D polyfaces</source>
      <translation>Yüzler içeren tüm nesneler 3B çokluyüzler olarak dışa aktarılacak</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="570"/>
      <source>Export 3D objects as polyface meshes</source>
      <translation>3B nesneleri, ÇokluYüzey (Polyface) kafesler olarak dışa aktar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="587"/>
      <source>Drawing Views will be exported as blocks.
This might fail for post DXF R12 templates.</source>
      <translation>Teknik Resim Görünümleri bloklar olarak dışa aktarılacaktır.
Bu, DXF R12 şablonları sonrası için başarısız olabilir.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="591"/>
      <source>Export Drawing Views as blocks</source>
      <translation>Çizim Görünümlerini blok olarak dışa aktar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="611"/>
      <source>Exported objects will be projected to reflect the current view direction</source>
      <translation>Dışa aktarılan nesneler, mevcut görünüm yönünün yansıması şekilde olacaktır</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="614"/>
      <source>Project exported objects along current view direction</source>
      <translation>Proje nesneleri geçerli görünüm yönü boyunca dışa</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="14"/>
      <source>Grid and snapping</source>
      <translation>Izgara ve Yakalama</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="35"/>
      <source>Snapping</source>
      <translation>Yakalama</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="43"/>
      <source>If this is checked, snapping is activated without the need to press the snap mod key</source>
      <translation>Eğer seçiliyse, yakalama modu tuşuna basmak zorunda kalmadan, yakalama etkinleştirilir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="46"/>
      <source>Always snap (disable snap mod)</source>
      <translation>Unsurları her zaman yakala (unsur yakalama modu devre dışı)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="66"/>
      <source>Constrain mod</source>
      <translation>Kısıtlama modu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="86"/>
      <source>The Constraining modifier key</source>
      <translation>Kısıtlayıcı değiştirme tuşu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="96"/>
      <location filename="../ui/preferences-draftsnap.ui" line="151"/>
      <location filename="../ui/preferences-draftsnap.ui" line="206"/>
      <source>Shift</source>
      <translation>Üst karakter</translation>
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
      <translation>Yakalama Modu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="138"/>
      <source>The snap modifier key</source>
      <translation>Yakalama değiştirme tuşu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="173"/>
      <source>Alt mod</source>
      <translation>Alt mod</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="193"/>
      <source>The Alt modifier key</source>
      <translation>Alt değiştirici tuşu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="228"/>
      <source>If checked, the Snap toolbar will be shown whenever you use snapping</source>
      <translation>Bu onay kutusu işaretliyse, yakalama araç çubuğu, yakalama özelliği her kullanıldığında görüntülenecektir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="231"/>
      <source>Show Draft Snap toolbar</source>
      <translation>Taslak yakalama araç çubuğunu göster</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="251"/>
      <source>Hide Draft snap toolbar after use</source>
      <translation>Kullanımdan sonra taslak yakalama araç çubuğunu gizle</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="272"/>
      <source>Grid</source>
      <translation>Izgara</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="278"/>
      <source>If checked, a grid will appear when drawing</source>
      <translation>Bu onay kutusu işaretlendiğinde, çizim yapılırken bir ızgara görünür</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="281"/>
      <source>Use grid</source>
      <translation>Izgara kullan</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="300"/>
      <source>If checked, the Draft grid will always be visible when the Draft workbench is active. Otherwise only when using a command</source>
      <translation>Eğer işaretliyse, Taslak tezgahı etkin olduğunda Taslak ızgarası(klavuzu) daima görünürdür. Aksi taktirde sadece bir komut kullanıldığında görünür</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="303"/>
      <source>Always show the grid</source>
      <translation>Her zaman başlık göster</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="319"/>
      <source>If checked, an additional border is displayed around the grid, showing the main square size in the bottom left border</source>
      <translation>İşaretlenirse, ızgaranın etrafında, sol alt sınırda ana kare boyutunu gösteren ek bir kenarlık görüntülenir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="322"/>
      <source>Show grid border</source>
      <translation>Izgara sınırını göster</translation>
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
      <translation>Izgaranın iki ana ekseni ayarlandığında genel eksenlerle eşleştiklerinde kırmızı, yeşil veya mavi renkte olacaktır</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="360"/>
      <source>Use colored axes</source>
      <translation>Renklendirilmiş aks kullan</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="381"/>
      <source>Main lines every</source>
      <translation>Ana çizgilerin tümü</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="404"/>
      <source>Mainlines will be drawn thicker. Specify here how many squares between mainlines.</source>
      <translation>Ana çizgiler daha kalın çizilecektir. Şimdi burada ana çizgiler arasında kaç kare olacağını belirleyiniz.</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="430"/>
      <source>Grid spacing</source>
      <translation>Izgara aralığı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="453"/>
      <source>The spacing between each grid line</source>
      <translation>Her bir kılavuz çizgisi arasındaki mesafe</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="485"/>
      <source>Grid size</source>
      <translation>Izgara boyutu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="505"/>
      <source>The number of horizontal or vertical lines of the grid</source>
      <translation>Yatay ve dikey kılavuz çizgisi sayısı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="511"/>
      <source> lines</source>
      <translation> çizgiler</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="534"/>
      <source>Grid color and transparency</source>
      <translation>Izgara rengi ve saydamlığı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="554"/>
      <source>The color of the grid</source>
      <translation>Izgaranın rengi</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="574"/>
      <source>The overall transparency of the grid</source>
      <translation>Izgaranın genel saydamlığı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="595"/>
      <source>Draft Edit preferences</source>
      <translation>Taslak Düzenleme tercihleri</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="598"/>
      <source>Edit</source>
      <translation>Düzenle</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="621"/>
      <source>Maximum number of contemporary edited objects</source>
      <translation>En fazla eş zamanlı düzenlenmiş nesne sayısı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="644"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Sets the maximum number of objects Draft Edit&lt;/p&gt;&lt;p&gt;can process at the same time&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Taslak Düzenleme'nin aynı anda işleyebileceği &lt;/p&gt;&lt;p&gt;en fazla nesne sayısını ayarlar&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="691"/>
      <source>Draft edit pick radius</source>
      <translation>Taslak düzenleme seçim yarıçapı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="714"/>
      <source>Controls pick radius of edit nodes</source>
      <translation>Düzenleme düğümlerinin yarıçapını kontrol eder</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="14"/>
      <source>DWG</source>
      <translation>DWG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="26"/>
      <source>DWG conversion</source>
      <translation>DWG dönüşümü</translation>
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
      <translation>Otomatik</translation>
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
      <translation>&lt;html&gt;&lt;head/&gt; &lt;body&gt;&lt;p&gt;&lt;span style="font-weight:600;"&gt; Not:&lt;/span&gt; DXF seçenekleri de DWG dosyaları için geçerlidir.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="14"/>
      <source>User interface settings</source>
      <translation>Kullanıcı arayüzü ayarları</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="26"/>
      <source>In-Command Shortcuts</source>
      <translation>Komut-İçi Kısayollar</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="37"/>
      <source>Relative</source>
      <translation>Göreceli</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="59"/>
      <source>R</source>
      <translation>R</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="81"/>
      <source>Continue</source>
      <translation>Devam</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="103"/>
      <source>T</source>
      <translation>T</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="125"/>
      <source>Close</source>
      <translation>Kapat</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="147"/>
      <source>O</source>
      <translation>O</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="169"/>
      <source>Copy</source>
      <translation>Kopyala</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="191"/>
      <source>P</source>
      <translation>P</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="213"/>
      <source>Subelement Mode</source>
      <translation>Alt Seçim Modu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="235"/>
      <source>D</source>
      <translation>D</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="257"/>
      <source>Fill</source>
      <translation>Doldur</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="279"/>
      <source>L</source>
      <translation>L</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="301"/>
      <source>Exit</source>
      <translation>Çıkış</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="323"/>
      <source>A</source>
      <translation>A</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="345"/>
      <source>Select Edge</source>
      <translation>Kenar seç</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="367"/>
      <source>E</source>
      <translation>E</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="389"/>
      <source>Add Hold</source>
      <translation>Tutamak Ekle</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="411"/>
      <source>Q</source>
      <translation>Q</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="433"/>
      <source>Length</source>
      <translation>Uzunluk</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="455"/>
      <source>H</source>
      <translation>H</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="477"/>
      <source>Wipe</source>
      <translation>Temizle</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="499"/>
      <source>W</source>
      <translation>W</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="521"/>
      <source>Set WP</source>
      <translation>WP ayarla</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="543"/>
      <source>U</source>
      <translation>U</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="565"/>
      <source>Cycle Snap</source>
      <translation>Döngü Yakala</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="587"/>
      <source>`</source>
      <translation>`</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="609"/>
      <source>Global</source>
      <translation>Küresel</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="631"/>
      <source>G</source>
      <translation>G</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="653"/>
      <source>Snap</source>
      <translation>Yakala</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="675"/>
      <source>S</source>
      <translation>S</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="697"/>
      <source>Increase Radius</source>
      <translation>Yarıçapı artır</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="719"/>
      <source>[</source>
      <translation>[</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="741"/>
      <source>Decrease Radius</source>
      <translation>Yarıçapı Azalt</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="763"/>
      <source>]</source>
      <translation>]</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="785"/>
      <source>Restrict X</source>
      <translation>Kısıtla X</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="807"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="829"/>
      <source>Restrict Y</source>
      <translation>Kısıtla Y</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="851"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="873"/>
      <source>Restrict Z</source>
      <translation>Kısıtla Z</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="895"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="928"/>
      <source>Enable draft statusbar customization</source>
      <translation>Taslak durum çubuğu özelleştirmesini etkinleştir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="931"/>
      <source>Draft Statusbar</source>
      <translation>Taslak durum çubuğu</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="951"/>
      <source>Enable snap statusbar widget</source>
      <translation>Yakalama durum çubuğu araçlarını etkinleştir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="954"/>
      <source>Draft snap widget</source>
      <translation>Draft için yakalama aracı</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="970"/>
      <source>Enable draft statusbar annotation scale widget</source>
      <translation>Taslak durum çubuğu açıklama aracı ölçeğini etkinleştir</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="973"/>
      <source>Annotation scale widget</source>
      <translation>Açıklama aracı ölçeği</translation>
    </message>
  </context>
  <context>
    <name>ImportAirfoilDAT</name>
    <message>
      <location filename="../../importAirfoilDAT.py" line="193"/>
      <source>Did not find enough coordinates</source>
      <translation>Yeterli koordinat bulunamadı</translation>
    </message>
  </context>
  <context>
    <name>ImportSVG</name>
    <message>
      <location filename="../../importSVG.py" line="1796"/>
      <source>Unknown SVG export style, switching to Translated</source>
      <translation>Bilinmeyen SVG dışa aktarma sitili, çevirme yegeç</translation>
    </message>
    <message>
      <location filename="../../importSVG.py" line="1816"/>
      <source>The export list contains no object with a valid bounding box</source>
      <translation>Dışa aktarma listesi geçerli bir sınırlayıcı kutuya sahip hiçbir nesne içermiyor</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../InitGui.py" line="104"/>
      <source>Draft creation tools</source>
      <translation>Taslak hazırlama araçları</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="107"/>
      <source>Draft annotation tools</source>
      <translation>Taslak açıklama araçları</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="110"/>
      <source>Draft modification tools</source>
      <translation>Taslak düzenleme araçları</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="113"/>
      <source>Draft utility tools</source>
      <translation>Yararlı aletler tasarlayın</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="118"/>
      <source>&amp;Drafting</source>
      <translation>&amp;Taslak hazırlama</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="121"/>
      <source>&amp;Annotation</source>
      <translation>&amp;Açıklayıcı notlar</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="124"/>
      <source>&amp;Modification</source>
      <translation>&amp;Değiştir</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="127"/>
      <source>&amp;Utilities</source>
      <translation>&amp;Araçlar</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="50"/>
      <source>Arc tools</source>
      <translation>Yay Araçları</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="58"/>
      <source>Bézier tools</source>
      <translation type="unfinished">Bézier tools</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="89"/>
      <source>Array tools</source>
      <translation>Sıralama araçları</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snapper.py" line="1477"/>
      <source>Draft Snap</source>
      <translation>Taslak Yakala</translation>
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
      <translation>FreeCAD tarafından işleme konması gereken DXF içe / dışa aktarma kütüphaneleri
bu sistemde DXF biçimi bulunamadı.
Lütfen FreeCAD'in bu kitaplıkları indirmesini sağlayın:
   1 - Taslak tezgah yükleyin
   2 - Menü Düzenleme&gt; Tercihler&gt; Alma-Dışa Aktarma&gt; DXF&gt; Yüklemeleri etkinleştir
Veya üzerinde açıklandığı gibi bu kitaplıkları manuel olarak indirin
https://github.com/yorikvanhavre/Draft-dxf-importer
Bu kütüphaneleri indirmek için FreeCAD'i etkinleştirmek için Evet cevabını verin.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="57"/>
      <location filename="../../DraftGui.py" line="751"/>
      <source>Relative</source>
      <translation>Göreceli</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="61"/>
      <location filename="../../DraftGui.py" line="756"/>
      <source>Global</source>
      <translation>Küresel</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="66"/>
      <location filename="../../DraftGui.py" line="774"/>
      <location filename="../../DraftGui.py" line="1126"/>
      <source>Continue</source>
      <translation>Devam</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="71"/>
      <location filename="../../DraftGui.py" line="790"/>
      <source>Close</source>
      <translation>Kapat</translation>
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
      <translation>Kopyala</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="81"/>
      <source>Subelement mode</source>
      <translation>AltSeçim modu</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="86"/>
      <source>Fill</source>
      <translation>Doldur</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="91"/>
      <source>Exit</source>
      <translation>Çıkış</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="96"/>
      <source>Snap On/Off</source>
      <translation>Yakalama Açık/Kapalı</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="101"/>
      <source>Increase snap radius</source>
      <translation>Yakalama yarıçapını artırın</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="106"/>
      <source>Decrease snap radius</source>
      <translation>Yakalama yarıçapını azaltın</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="111"/>
      <source>Restrict X</source>
      <translation>Kısıtla X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="116"/>
      <source>Restrict Y</source>
      <translation>Kısıtla Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="121"/>
      <source>Restrict Z</source>
      <translation>Kısıtla Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="126"/>
      <location filename="../../DraftGui.py" line="796"/>
      <source>Select edge</source>
      <translation>Kenarı seç</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="131"/>
      <source>Add custom snap point</source>
      <translation>Özel yakalama noktası ekle</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="136"/>
      <source>Length mode</source>
      <translation>Uzunluk modu</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="141"/>
      <location filename="../../DraftGui.py" line="792"/>
      <source>Wipe</source>
      <translation>Temizle</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="146"/>
      <source>Set Working Plane</source>
      <translation>Çalışma Düzlemini Ayarla</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="151"/>
      <source>Cycle snap object</source>
      <translation>Döngüsel yakalama nesnesi</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="156"/>
      <source>Toggle near snap on/off</source>
      <translation>Yakını yakalamayı aç veya kapa olarak değiştir</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="330"/>
      <source>Draft Command Bar</source>
      <translation>Taslak komut çubuğu</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="659"/>
      <location filename="../../WorkingPlane.py" line="821"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="374"/>
      <source>Top</source>
      <translation>üst</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="661"/>
      <location filename="../../WorkingPlane.py" line="832"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="386"/>
      <source>Front</source>
      <translation>Ön</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="663"/>
      <location filename="../../WorkingPlane.py" line="843"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="398"/>
      <source>Side</source>
      <translation>Yan</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="665"/>
      <source>Auto</source>
      <translation>Otomatik</translation>
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
      <translation>Hiçbiri</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="728"/>
      <source>active command:</source>
      <translation>Etkin söz dizisi:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="730"/>
      <source>Active Draft command</source>
      <translation>Aktif Çizim komutu</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="731"/>
      <source>X coordinate of next point</source>
      <translation>Bir sonraki noktanın X koordinatı</translation>
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
      <translation>Bir sonraki noktanın Y koordinatı</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="736"/>
      <source>Z coordinate of next point</source>
      <translation>Bir sonraki noktanın Z koordinatı</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="737"/>
      <source>Enter point</source>
      <translation>Nokta gir</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="739"/>
      <source>Enter a new point with the given coordinates</source>
      <translation>Yeni bir noktası verilen koordinatlarla girin</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="740"/>
      <source>Length</source>
      <translation>Uzunluk</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="741"/>
      <location filename="../../draftguitools/gui_trimex.py" line="220"/>
      <source>Angle</source>
      <translation>Açı</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="742"/>
      <source>Length of current segment</source>
      <translation>Geçerli kesimin uzunluğu</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="743"/>
      <source>Angle of current segment</source>
      <translation>Geçerli kesimin uzunluğu</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="747"/>
      <source>Check this to lock the current angle</source>
      <translation>Geçerli açıyı kilitlemek için bunu kontrol edin</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="748"/>
      <location filename="../../DraftGui.py" line="1108"/>
      <source>Radius</source>
      <translation>Yarıçap</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="749"/>
      <location filename="../../DraftGui.py" line="1109"/>
      <source>Radius of Circle</source>
      <translation>Çemberin Yarıçapı</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="754"/>
      <source>Coordinates relative to last point or to coordinate system origin
if is the first point to set</source>
      <translation>Eğer ilk nokta ayarlı ise koordinatlar, koordinat sistemi orijinine
veya son noktaya bağlıdır</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="759"/>
      <source>Coordinates relative to global coordinate system.
Uncheck to use working plane coordinate system</source>
      <translation>Koordinatlar, küresel koordinat sistemine bağlıdır. Çalışma 
düzlemi koordinat sistemini kullanmak için işareti kaldırın</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="761"/>
      <source>Filled</source>
      <translation>Dolu</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="765"/>
      <source>Check this if the object should appear as filled, otherwise it will appear as wireframe.
Not available if Draft preference option 'Use Part Primitives' is enabled</source>
      <translation>Check this if the object should appear as filled, otherwise it will appear as wireframe.
Not available if Draft preference option 'Use Part Primitives' is enabled</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="767"/>
      <source>Finish</source>
      <translation>Bitir</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="769"/>
      <source>Finishes the current drawing or editing operation</source>
      <translation>Geçerli çizim veya düzenleme işlemini sonlandırır</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="772"/>
      <source>If checked, command will not finish until you press the command button again</source>
      <translation>Eğer işaretliyse, komut tekrar komut tuşuna basana kadar bitmeyecek</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="777"/>
      <source>If checked, an OCC-style offset will be performedinstead of the classic offset</source>
      <translation>If checked, an OCC-style offset will be performedinstead of the classic offset</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="778"/>
      <source>&amp;OCC-style offset</source>
      <translation>&amp;amp; OCC tarzı ofset</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="788"/>
      <source>&amp;Undo (CTRL+Z)</source>
      <translation>Geri Al (Ctrl+Z)</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="789"/>
      <source>Undo the last segment</source>
      <translation>Son segmenti geri al</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="791"/>
      <source>Finishes and closes the current line</source>
      <translation>Geçerli çizgiyi kapatır ve sonlandırır</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="793"/>
      <source>Wipes the existing segments of this line and starts again from the last point</source>
      <translation>Bu çizginin mevcut bölümlerini siler ve son noktadan tekrar başlar</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="794"/>
      <source>Set WP</source>
      <translation>WP ayarla</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="795"/>
      <source>Reorients the working plane on the last segment</source>
      <translation>Son segmentteki çalışma düzlemini yeniden yönlendirir</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="797"/>
      <source>Selects an existing edge to be measured by this dimension</source>
      <translation>Bu ebat tarafından ölçülecek mevcut bir kenar seçer</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="798"/>
      <source>Sides</source>
      <translation>Kenarlar</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="799"/>
      <source>Number of sides</source>
      <translation>Yüzlerin sayısı</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="802"/>
      <source>If checked, objects will be copied instead of moved. Preferences -&gt; Draft -&gt; Global copy mode to keep this mode in next commands</source>
      <translation>İşaretlenirse, taşınacaklar yerine nesneler kopyalanacaktır. Tercihler -&gt; Taslak -&gt; Evrensel Kopyalama modu için bu modu sonraki komutlarda sürdür</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="803"/>
      <source>Modify subelements</source>
      <translation>AltElemanları değiştir</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="804"/>
      <source>If checked, subelements will be modified instead of entire objects</source>
      <translation>İşaretlenirse, nesnelerin tamamı yerine alt öğeler değiştirilir</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="805"/>
      <source>Text string to draw</source>
      <translation>Çizmek için metin dizesi</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="806"/>
      <source>String</source>
      <translation>Dize</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="807"/>
      <source>Height of text</source>
      <translation>Satır yüksekliği</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="808"/>
      <source>Height</source>
      <translation>Yükseklik</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="809"/>
      <source>Intercharacter spacing</source>
      <translation>Arası karakter aralığı</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="810"/>
      <source>Tracking</source>
      <translation>İzleme</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="811"/>
      <source>Full path to font file:</source>
      <translation>Yazı tipi dosyasının tam yolu:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="812"/>
      <source>Open a FileChooser for font file</source>
      <translation>Bir FileChooser için yazı tipi dosyası aç</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="813"/>
      <source>Create text</source>
      <translation>Yazı oluştur</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="814"/>
      <source>Press this button to create the text object, or finish your text with two blank lines</source>
      <translation>Metin nesnesini oluşturmak veya metninizi iki boş satırla bitirmek için bu düğmeye basın</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="836"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="272"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="327"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="530"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="541"/>
      <source>Current working plane</source>
      <translation>Mevcut çalışma düzlemi</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="837"/>
      <source>Change default style for new objects</source>
      <translation>Yeni nesneler için varsayılan biçimi değiştir</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="838"/>
      <source>Toggle construction mode</source>
      <translation>İnşa Modunu Değiştir</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="839"/>
      <location filename="../../DraftGui.py" line="2050"/>
      <location filename="../../DraftGui.py" line="2065"/>
      <source>Autogroup off</source>
      <translation>Autogroup kapalı</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="950"/>
      <source>Line</source>
      <translation>Çizgi</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="958"/>
      <source>DWire</source>
      <translation>DWire</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="976"/>
      <source>Circle</source>
      <translation>Daire</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="981"/>
      <source>Arc</source>
      <translation>Yay</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="986"/>
      <location filename="../../draftguitools/gui_rotate.py" line="286"/>
      <source>Rotate</source>
      <translation>Döndür</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="990"/>
      <source>Point</source>
      <translation>Nokta</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1018"/>
      <source>Label</source>
      <translation>Etiket</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1036"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="527"/>
      <location filename="../../draftguitools/gui_offset.py" line="243"/>
      <location filename="../../draftguitools/gui_offset.py" line="260"/>
      <location filename="../../draftguitools/gui_offset.py" line="324"/>
      <source>Offset</source>
      <translation>Uzaklaşma</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1042"/>
      <location filename="../../DraftGui.py" line="1100"/>
      <location filename="../../draftguitools/gui_trimex.py" line="215"/>
      <source>Distance</source>
      <translation>Uzaklık</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1043"/>
      <location filename="../../DraftGui.py" line="1101"/>
      <location filename="../../draftguitools/gui_trimex.py" line="217"/>
      <source>Offset distance</source>
      <translation>Öteleme mesafesi</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1097"/>
      <source>Trimex</source>
      <translation>KırpUzat</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1197"/>
      <source>Pick Object</source>
      <translation>Nesne Seç</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1203"/>
      <source>Edit</source>
      <translation>Düzenle</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1253"/>
      <source>Local u0394X</source>
      <translation>Yerel u0394X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1254"/>
      <source>Local u0394Y</source>
      <translation>Yerel u0394Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1255"/>
      <source>Local u0394Z</source>
      <translation>Yerel u0394Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1257"/>
      <source>Local X</source>
      <translation>Yerel X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1258"/>
      <source>Local Y</source>
      <translation>Yerel Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1259"/>
      <source>Local Z</source>
      <translation>Yerel Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1261"/>
      <source>Global u0394X</source>
      <translation>Küresel u0394X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1262"/>
      <source>Global u0394Y</source>
      <translation>Küresel u0394Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1263"/>
      <source>Global u0394Z</source>
      <translation>Küresel u0394Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1265"/>
      <source>Global X</source>
      <translation>Evrensel X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1266"/>
      <source>Global Y</source>
      <translation>Evrensel Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1267"/>
      <source>Global Z</source>
      <translation>Evrensel Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1503"/>
      <source>Invalid Size value. Using 200.0.</source>
      <translation>Geçersiz boyut değeri. 200.0 kullanarak.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1511"/>
      <source>Invalid Tracking value. Using 0.</source>
      <translation>Geçersiz izleme değeri. 0 kullanarak.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1525"/>
      <source>Please enter a text string.</source>
      <translation>Lütfen geçerli bir Url dizesi girin.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1534"/>
      <source>Select a Font file</source>
      <translation>Font dosyası seç</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1567"/>
      <source>Please enter a font file.</source>
      <translation>Lütfen geçerli bir başlık giriniz.</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2058"/>
      <source>Autogroup:</source>
      <translation>Oto-grup:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2394"/>
      <source>Faces</source>
      <translation>Yüzler</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2395"/>
      <source>Remove</source>
      <translation>Kaldır</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2396"/>
      <source>Add</source>
      <translation>Ekle</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2397"/>
      <source>Facebinder elements</source>
      <translation>Yüzey bağlama elemanları</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="46"/>
      <source>Draft</source>
      <translation>Taslak</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="209"/>
      <location filename="../../importDWG.py" line="281"/>
      <source>LibreDWG error</source>
      <translation>LibreDWG error</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="218"/>
      <location filename="../../importDWG.py" line="290"/>
      <source>Converting:</source>
      <translation>Dönüştürülüyor:</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="223"/>
      <source>Conversion successful</source>
      <translation>Dönüştürme başarılı</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="226"/>
      <source>Error during DWG conversion. Try moving the DWG file to a directory path without spaces and non-english characters, or try saving to a lower DWG version.</source>
      <translation>Error during DWG conversion. Try moving the DWG file to a directory path without spaces and non-english characters, or try saving to a lower DWG version.</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="229"/>
      <location filename="../../importDWG.py" line="296"/>
      <source>ODA File Converter not found</source>
      <translation>ODA File Converter not found</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="242"/>
      <location filename="../../importDWG.py" line="306"/>
      <source>QCAD error</source>
      <translation>OCC hatası</translation>
    </message>
    <message>
      <location filename="../../WorkingPlane.py" line="713"/>
      <location filename="../../draftmake/make_sketch.py" line="127"/>
      <location filename="../../draftmake/make_sketch.py" line="139"/>
      <source>All Shapes must be coplanar</source>
      <translation>Tüm Şekiller aynı düzlemde olmalıdır</translation>
    </message>
    <message>
      <location filename="../../WorkingPlane.py" line="721"/>
      <source>Selected Shapes must define a plane</source>
      <translation>Selected Shapes must define a plane</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="81"/>
      <source>No graphical interface</source>
      <translation>Görsel arayüz yok</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="161"/>
      <source>Unable to insert new object into a scaled part</source>
      <translation>Ölçeklenmiş bir parçaya yeni nesne eklenemez</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="267"/>
      <source>Symbol not implemented. Using a default symbol.</source>
      <translation>Simge oluşturulmamış. Varsayılan simge kullanılıyor.</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="333"/>
      <source>Visibility off; removed from list: </source>
      <translation>Görünürlük kapalı; listeden kaldırıldı: </translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="603"/>
      <source>image is Null</source>
      <translation>görüntü boş</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="609"/>
      <source>filename does not exist on the system or in the resource file</source>
      <translation>dosya adı, sistem üzerinde veya kaynak dosyada bulunmuyor</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="668"/>
      <source>unable to load texture</source>
      <translation>doku yüklenemiyor</translation>
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
      <translation>Etkin belge yok. Durduruluyor.</translation>
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
      <translation>Yanlış girdi: nesne belgede yok.</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="738"/>
      <source>Does not have 'ViewObject.RootNode'.</source>
      <translation>'ViewObject.RootNode' a sahip değil.</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="51"/>
      <location filename="../../draftutils/init_draft_statusbar.py" line="58"/>
      <location filename="../../draftutils/init_draft_statusbar.py" line="65"/>
      <location filename="../../draftutils/init_draft_statusbar.py" line="154"/>
      <source>custom</source>
      <translation>özel</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="140"/>
      <source>Unable to convert input into a  scale factor</source>
      <translation>Girdi bir ölçek faktörüne dönüştürülemiyor</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="155"/>
      <source>Set custom scale</source>
      <translation>Özel ölçek ayarla</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="157"/>
      <source>Set custom annotation scale in format x:x, x=x</source>
      <translation>Özel açıklama ölçeğini x: x, x = x biçiminde ayarlayın</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="214"/>
      <source>Set the scale used by draft annotation tools</source>
      <translation>Draft açıklama araçları tarafından kullanılan ölçeği ayarlayın</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="650"/>
      <source>Solids:</source>
      <translation>Katılar:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="651"/>
      <source>Faces:</source>
      <translation>Yüzler:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="652"/>
      <source>Wires:</source>
      <translation>Tel Kafesler:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="653"/>
      <source>Edges:</source>
      <translation>Kenarlar:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="654"/>
      <source>Vertices:</source>
      <translation>Köşeler:</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="658"/>
      <source>Face</source>
      <translation>Yüz</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="663"/>
      <source>Wire</source>
      <translation>Yay</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="695"/>
      <location filename="../../draftutils/utils.py" line="699"/>
      <source>different types</source>
      <translation>farklı türler</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="709"/>
      <source>Objects have different placements. Distance between the two base points: </source>
      <translation>Nesneler farklı yerleşimlere sahip. İki uç nokta arası mesafe: </translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="712"/>
      <source>has a different value</source>
      <translation>farklı bir değere sahip</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="715"/>
      <source>doesn't exist in one of the objects</source>
      <translation>nesnelerin birinde bulunmuyor</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="827"/>
      <source>%s shares a base with %d other objects. Please check if you want to modify this.</source>
      <translation>%s, diğer %d nesneyle bir tabanı paylaşıyor. Lütfen bunu değiştirmek isteyip istemediğinizi kontrol edin.</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="833"/>
      <source>%s cannot be modified because its placement is readonly.</source>
      <translation>%s değiştirilemiyor çünkü salt okunur yerleşime sahip.</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="977"/>
      <source>Wrong input: unknown document.</source>
      <translation>Yanlış girdi: bilinmeyen belge.</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="1055"/>
      <source>This function will be deprecated in </source>
      <translation>Bu işlev şunda artık kullanılmayacak: </translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="1056"/>
      <location filename="../../draftutils/utils.py" line="1059"/>
      <source>Please use </source>
      <translation>Lüften bunu kullan: </translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="1059"/>
      <source>This function will be deprecated. </source>
      <translation>Bu işlev artık kullanılmayacak. </translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="169"/>
      <source>Snap Lock</source>
      <translation>Kilit Çıtçıtı</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="170"/>
      <source>Snap Endpoint</source>
      <translation>Bitiş Noktası Çıtçıtı</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="171"/>
      <source>Snap Midpoint</source>
      <translation>Orta Nokta Çıtçıtı</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="172"/>
      <source>Snap Center</source>
      <translation>Merkez Çıtçıtı</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="173"/>
      <source>Snap Angle</source>
      <translation>Açı Çıtçıtı</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="174"/>
      <source>Snap Intersection</source>
      <translation>Kesişim Çıtçıtı</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="175"/>
      <source>Snap Perpendicular</source>
      <translation>Dik Yakala</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="176"/>
      <source>Snap Extension</source>
      <translation>Uzantıyı Yakala</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="177"/>
      <source>Snap Parallel</source>
      <translation>Paraleli Yakala</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="178"/>
      <source>Snap Special</source>
      <translation>Özel Yakala</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="179"/>
      <source>Snap Near</source>
      <translation>Yakın Çıtçıtı</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="180"/>
      <source>Snap Ortho</source>
      <translation>Orto Çıtçıtı</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="181"/>
      <source>Snap Grid</source>
      <translation>Izgara Çıtçıtı</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="182"/>
      <source>Snap WorkingPlane</source>
      <translation>Çalışma Düzlemi Çıtçıtı</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="183"/>
      <source>Snap Dimensions</source>
      <translation>Boyutlar Çıtçıtı</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="187"/>
      <source>Toggle Draft Grid</source>
      <translation>Taslak Izgarasını Aç/Kapat</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="69"/>
      <source>ShapeString: string has no wires</source>
      <translation>ShapeString: dize tel içermiyor</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="89"/>
      <location filename="../../draftobjects/draft_annotation.py" line="105"/>
      <source>added view property 'ScaleMultiplier'</source>
      <translation>'ÖlçekÇarpanı' görünüm özelliği eklendi</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="125"/>
      <location filename="../../draftobjects/draft_annotation.py" line="130"/>
      <source>migrated 'DraftText' type to 'Text'</source>
      <translation>"TaslakMetni" türü "Metin" e taşındı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="284"/>
      <source>, path object doesn't have 'Edges'.</source>
      <translation>, yol nesnesinin 'Kenarları' yoktur.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="395"/>
      <location filename="../../draftobjects/patharray.py" line="401"/>
      <location filename="../../draftobjects/patharray.py" line="407"/>
      <source>'PathObj' property will be migrated to 'PathObject'</source>
      <translation>'PathObj' özelliği 'PathObject'e yükseltilecek</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="525"/>
      <source>Cannot calculate path tangent. Copy not aligned.</source>
      <translation>Yol tanjantı hesaplanamıyor. Kopya hizalanmadı.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="541"/>
      <source>Tangent and normal are parallel. Copy not aligned.</source>
      <translation>Teğet ve normal paraleldir. Kopya hizalanmadı.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="558"/>
      <source>Cannot calculate path normal, using default.</source>
      <translation>Varsayılan kullanılarak yol normali hesaplanamaz.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="565"/>
      <source>Cannot calculate path binormal. Copy not aligned.</source>
      <translation>Yol binormali hesaplanamıyor. Kopya hizalanmadı.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="571"/>
      <source>AlignMode {} is not implemented</source>
      <translation>HizalamaModu {} uygulanmadı</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="145"/>
      <location filename="../../draftobjects/pointarray.py" line="161"/>
      <source>added property 'ExtraPlacement'</source>
      <translation>'EkstraYerleşim' özelliği eklendi</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="151"/>
      <source>Object must be a closed shape</source>
      <translation>Nesne kapalı bir şekil olmalıdır</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="153"/>
      <source>No solid object created</source>
      <translation>Katı nesne oluşturulmadı</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="276"/>
      <source>Faces must be coplanar to be refined</source>
      <translation>Yüzlerin düzeltilmesi için eş düzlemli olması gerekir</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="435"/>
      <location filename="../../draftfunctions/downgrade.py" line="230"/>
      <source>Upgrade: Unknown force method:</source>
      <translation>Yükseltme: Bilinmeyen kuvvet yöntemi:</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="453"/>
      <source>Found groups: closing each open object inside</source>
      <translation>Bulunan gruplar: içindeki her açık nesneyi kapatılıyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="459"/>
      <source>Found meshes: turning into Part shapes</source>
      <translation>Bulunan Mesh(ler): Parça şekillerine dönüşüyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="467"/>
      <source>Found 1 solidifiable object: solidifying it</source>
      <translation>Katılaştırılabilir 1 nesne bulundu: Katılaştırılıyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="472"/>
      <source>Found 2 objects: fusing them</source>
      <translation>2 Nesne bulundu: birleştiriliyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="483"/>
      <source>Found object with several coplanar faces: refine them</source>
      <translation>Birkaç düzlemsel yüzeye sahip nesne bulundu: onları arıtın</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="489"/>
      <source>Found 1 non-parametric objects: draftifying it</source>
      <translation>Parametrik olmayan 1 nesne bulundu: taslağı oluşturun</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="500"/>
      <source>Found 1 closed sketch object: creating a face from it</source>
      <translation>Kapalı 1 eskiz nesnesi bulundu: ondan bir yüzey oluşturuluyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="505"/>
      <source>Found closed wires: creating faces</source>
      <translation>Kapalı teller/kafesler bulundu: Yüzeyler oluşturuluyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="511"/>
      <source>Found several wires or edges: wiring them</source>
      <translation>Birkaç kenar bulundu: onlar Telleniyor (kafesleniyor)</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="513"/>
      <location filename="../../draftfunctions/upgrade.py" line="547"/>
      <source>Found several non-treatable objects: creating compound</source>
      <translation>Birkaç iyileştirilemeyen nesne bulundu: Bileşik oluşturuluyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="518"/>
      <source>trying: closing it</source>
      <translation>deneniyor: kapatılıyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="520"/>
      <source>Found 1 open wire: closing it</source>
      <translation>1 açık tel bulundu: kapatılıyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="537"/>
      <source>Found 1 object: draftifying it</source>
      <translation>1 nesne bulundu: taslak haline getiriliyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="542"/>
      <source>Found points: creating compound</source>
      <translation>Bulunan noktalar: bileşik oluşturuluyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="550"/>
      <source>Unable to upgrade these objects.</source>
      <translation>Bu nesneler yükseltilemiyor.</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="90"/>
      <source>No object given</source>
      <translation>Verilen nesne yok</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="94"/>
      <source>The two points are coincident</source>
      <translation>İki nokta çakışıyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="113"/>
      <source>mirrored</source>
      <translation>yansıtılmış</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="238"/>
      <source>Found 1 block: exploding it</source>
      <translation>1 blok bulundu: patlatılıyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="246"/>
      <source>Found 1 multi-solids compound: exploding it</source>
      <translation>1 çoklu-katı bileşiği bulundu: patlatılıyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="253"/>
      <source>Found 1 parametric object: breaking its dependencies</source>
      <translation>1 parametrik nesne bulundu: bağımlılıklarını ortadan kaldırıyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="261"/>
      <source>Found 2 objects: subtracting them</source>
      <translation>2 nesne bulundu: bulunanlar çıkarılıyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="268"/>
      <source>Found several faces: splitting them</source>
      <translation>Birkaç yüzey bulundu: onlar ayıklanıyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="273"/>
      <source>Found several objects: subtracting them from the first one</source>
      <translation>Birçok nesne bulundu: onlar ilkinden çıkarılıyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="278"/>
      <source>Found 1 face: extracting its wires</source>
      <translation>1 yüzey bulundu: tellere ayrılııyorlar</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="284"/>
      <source>Found only wires: extracting their edges</source>
      <translation>Sadece teller bulundu: kenarları ayıklanıyor</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="288"/>
      <source>No more downgrade possible</source>
      <translation>Daha fazla derece düşürme mümkün değil</translation>
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
      <translation>Yanlış giriş: bir vektör olmalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_pointarray.py" line="147"/>
      <location filename="../../draftmake/make_text.py" line="107"/>
      <location filename="../../draftmake/make_label.py" line="215"/>
      <source>Wrong input: must be a placement, a vector, or a rotation.</source>
      <translation>Yanlış girdi: bir yerleşim, vektör veya dönüş olmalıdır.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="316"/>
      <location filename="../../draftmake/make_label.py" line="230"/>
      <source>Wrong input: object must not be a list.</source>
      <translation>Yanlış girdi: nesne bir liste olmamalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_patharray.py" line="213"/>
      <location filename="../../draftmake/make_label.py" line="251"/>
      <source>Wrong input: must be a list or tuple of strings, or a single string.</source>
      <translation>Yanlış girdi: bir liste veya karakter dizisi demeti veya tek bir karakter dizisi olmalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="263"/>
      <source>Wrong input: subelement not in object.</source>
      <translation>Yanlış girdi: alt eleman nesnede değil.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="272"/>
      <source>Wrong input: label_type must be a string.</source>
      <translation>Wrong input: label_type must be a string.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="277"/>
      <source>Wrong input: label_type must be one of the following: </source>
      <translation>Wrong input: label_type must be one of the following: </translation>
    </message>
    <message>
      <location filename="../../draftmake/make_text.py" line="91"/>
      <location filename="../../draftmake/make_text.py" line="96"/>
      <location filename="../../draftmake/make_label.py" line="286"/>
      <location filename="../../draftmake/make_label.py" line="291"/>
      <source>Wrong input: must be a list of strings or a single string.</source>
      <translation>Yanlış girdi: bir karakter dizi listesi veya tek bir karakter dizisi olmalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="300"/>
      <location filename="../../draftmake/make_label.py" line="304"/>
      <source>Wrong input: must be a string, 'Horizontal', 'Vertical', or 'Custom'.</source>
      <translation>Yanlış giriş: bir karakter dizisi, 'Yatay', 'Dikey' veya 'Özel' olmalıdır.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_polararray.py" line="119"/>
      <location filename="../../draftmake/make_layer.py" line="201"/>
      <location filename="../../draftmake/make_patharray.py" line="191"/>
      <location filename="../../draftmake/make_patharray.py" line="360"/>
      <location filename="../../draftmake/make_orthoarray.py" line="151"/>
      <location filename="../../draftmake/make_label.py" line="313"/>
      <source>Wrong input: must be a number.</source>
      <translation>Yanlış giriş: bir sayı olmalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="320"/>
      <source>Wrong input: must be a list of at least two vectors.</source>
      <translation>Yanlış girdi: en az iki vektörden oluşan bir liste olmalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="353"/>
      <source>Direction is not 'Custom'; points won't be used.</source>
      <translation>Yön 'Özel' değildir; nokta kullanılmayacak.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="380"/>
      <source>Wrong input: must be a list of two elements. For example, [object, 'Edge1'].</source>
      <translation>Yanlış girdi: iki öğeden oluşan bir liste olmalıdır. Örneğin, [nesne, 'Kenar1'].</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_pointarray.py" line="135"/>
      <source>Wrong input: point object doesn't have 'Geometry', 'Links', or 'Components'.</source>
      <translation>Yanlış girdi: nokta nesnesinde 'Geometri', 'Bağlantılar' veya 'Bileşenler' yok.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="69"/>
      <source>Layers</source>
      <translation>Katmanlar</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="145"/>
      <location filename="../../draftmake/make_layer.py" line="162"/>
      <location filename="../../draftguitools/gui_layers.py" line="47"/>
      <source>Layer</source>
      <translation>Katman</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="157"/>
      <source>Wrong input: it must be a string.</source>
      <translation>Yanlış girdi: bir karakter dizisi olmalıdır.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="167"/>
      <location filename="../../draftmake/make_layer.py" line="171"/>
      <location filename="../../draftmake/make_layer.py" line="184"/>
      <location filename="../../draftmake/make_layer.py" line="188"/>
      <source>Wrong input: must be a tuple of three floats 0.0 to 1.0.</source>
      <translation>Yanlış giriş: 0.0 ila 1.0 arasında üç basamaklı bir demet olmalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="208"/>
      <location filename="../../draftmake/make_layer.py" line="212"/>
      <source>Wrong input: must be 'Solid', 'Dashed', 'Dotted', or 'Dashdot'.</source>
      <translation>Yanlış giriş: 'Katı', 'Kesikli', 'Noktalı' veya 'ÇizgiNokta' olmalıdır.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="220"/>
      <source>Wrong input: must be a number between 0 and 100.</source>
      <translation>Yanlış giriş: 0 ile 100 arasında bir sayı olmalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="173"/>
      <source>This function is deprecated. Do not use this function directly.</source>
      <translation>Bu işlev kullanımdan kaldırılmıştır. Bu işlevi doğrudan kullanmayın.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="174"/>
      <source>Use one of 'make_linear_dimension', or 'make_linear_dimension_obj'.</source>
      <translation>'make_linear_dimension', veya 'make_linear_dimension_obj' den birini kullanın.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="327"/>
      <location filename="../../draftmake/make_dimension.py" line="452"/>
      <source>Wrong input: object doesn't have a 'Shape' to measure.</source>
      <translation>Yanlış girdi: nesnenin ölçülecek bir 'Şekli' yok.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="331"/>
      <source>Wrong input: object doesn't have at least one element in 'Vertexes' to use for measuring.</source>
      <translation>Yanlış girdi: nesnenin 'Köşelerde' ölçüm için kullanılacak en az bir öğesi yok.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="338"/>
      <location filename="../../draftmake/make_dimension.py" line="463"/>
      <source>Wrong input: must be an integer.</source>
      <translation>Yanlış giriş: bir tam sayı olmalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="343"/>
      <source>i1: values below 1 are not allowed; will be set to 1.</source>
      <translation>i1: 1'in altındaki değerlere izin verilmez; 1 olarak ayarlanacak.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="347"/>
      <location filename="../../draftmake/make_dimension.py" line="363"/>
      <source>Wrong input: vertex not in object.</source>
      <translation>Yanlış girdi: köşe nesnede değil.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="359"/>
      <source>i2: values below 1 are not allowed; will be set to the last vertex in the object.</source>
      <translation>i2: 1'in altındaki değerlere izin verilmez; nesnedeki son tepe noktasına ayarlanacaktır.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="456"/>
      <source>Wrong input: object doesn't have at least one element in 'Edges' to use for measuring.</source>
      <translation>Yanlış girdi: nesnenin 'Kenarlar'ında ölçüm için kullanılacak en az bir eleman yok.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="468"/>
      <source>index: values below 1 are not allowed; will be set to 1.</source>
      <translation>indeks: 1'in altındaki değerlere izin verilmez; 1 olarak ayarlanacak.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="472"/>
      <source>Wrong input: index doesn't correspond to an edge in the object.</source>
      <translation>Yanlış girdi: dizin, nesnedeki bir kenara karşılık gelmiyor.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="476"/>
      <source>Wrong input: index doesn't correspond to a circular edge.</source>
      <translation>Yanlış girdi: indeks dairesel bir kenara karşılık gelmiyor.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="483"/>
      <location filename="../../draftmake/make_dimension.py" line="487"/>
      <source>Wrong input: must be a string, 'radius' or 'diameter'.</source>
      <translation>Yanlış giriş: bir karaktre dizisi, "yarıçap" veya "çap" olmalıdır.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="579"/>
      <location filename="../../draftmake/make_dimension.py" line="586"/>
      <source>Wrong input: must be a list with two angles.</source>
      <translation>Yanlış giriş: iki açılı bir liste olmalıdır.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="60"/>
      <source>Internal orthogonal array</source>
      <translation>Dahili ortogonal dizilim</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="87"/>
      <source>Wrong input: must be a number or vector.</source>
      <translation>Yanlış giriş: bir sayı veya vektör olmalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="92"/>
      <location filename="../../draftmake/make_orthoarray.py" line="95"/>
      <location filename="../../draftmake/make_orthoarray.py" line="98"/>
      <source>Input: single value expanded to vector.</source>
      <translation>Girdi: vektöre genişletilmiş tek değer.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="154"/>
      <location filename="../../draftmake/make_polararray.py" line="112"/>
      <location filename="../../draftmake/make_orthoarray.py" line="119"/>
      <source>Wrong input: must be an integer number.</source>
      <translation>Yanlış giriş: bir tam sayı olmalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="123"/>
      <location filename="../../draftmake/make_orthoarray.py" line="126"/>
      <location filename="../../draftmake/make_orthoarray.py" line="129"/>
      <source>Input: number of elements must be at least 1. It is set to 1.</source>
      <translation>Giriş: eleman sayısı en az 1 olmalıdır. 1 olarak ayarlanmıştır.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="275"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="269"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="396"/>
      <source>Orthogonal array</source>
      <translation>Dikey dizi</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="350"/>
      <source>Orthogonal array 2D</source>
      <translation>Dikey 2D dizi</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="424"/>
      <source>Rectangular array</source>
      <translation>Dikdörtgenel dizi</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="501"/>
      <source>Rectangular array 2D</source>
      <translation>Dikdörtgenel 2D dizi</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_polararray.py" line="94"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="258"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="434"/>
      <source>Polar array</source>
      <translation>Kutupsal dizi</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_patharray.py" line="246"/>
      <source>Wrong input: must be 'Original', 'Frenet', or 'Tangent'.</source>
      <translation>Yanlış giriş: 'Orijinal', 'Frenet' veya 'Teğet' olmalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="125"/>
      <location filename="../../draftmake/make_arc_3points.py" line="130"/>
      <source>Points:</source>
      <translation>Noktalar:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="126"/>
      <location filename="../../draftmake/make_arc_3points.py" line="131"/>
      <source>Wrong input: must be list or tuple of three points exactly.</source>
      <translation>Yanlış giriş: tam olarak üç noktadan oluşan liste veya demet olmalıdır.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="138"/>
      <source>Placement:</source>
      <translation>Yerleşim:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="139"/>
      <source>Wrong input: incorrect type of placement.</source>
      <translation>Yanlış giriş: yanlış yerleşim türü.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="153"/>
      <source>Wrong input: incorrect type of points.</source>
      <translation>Yanlış giriş: yanlış nokta türü.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="159"/>
      <source>Cannot generate shape:</source>
      <translation>Şekil oluşturulamıyor:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="166"/>
      <source>Radius:</source>
      <translation>Yarıçap:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="167"/>
      <source>Center:</source>
      <translation>Merkez:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="170"/>
      <source>Create primitive object</source>
      <translation>Temel nesne oluştur</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="193"/>
      <location filename="../../draftmake/make_arc_3points.py" line="204"/>
      <source>Final placement:</source>
      <translation>Nihai yerleşim:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="195"/>
      <source>Face: True</source>
      <translation>Yüzey: Doğru</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="197"/>
      <source>Support:</source>
      <translation>Destek:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="198"/>
      <source>Map mode:</source>
      <translation>Harita modu:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_sketch.py" line="104"/>
      <source>No shape found</source>
      <translation>Bulundu şekil</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_sketch.py" line="111"/>
      <source>All Shapes must be planar</source>
      <translation>Tüm Şekiller aynı düzlemde olmalıdır</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="122"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="290"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="481"/>
      <source>Circular array</source>
      <translation>Dairesel dizi</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="144"/>
      <source>Wrong input: must be a number or quantity.</source>
      <translation>Yanlış girdi: bir sayı veya miktar olmalı.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="58"/>
      <location filename="../../draftmake/make_fillet.py" line="143"/>
      <location filename="../../draftmake/make_fillet.py" line="144"/>
      <location filename="../../draftmake/make_fillet.py" line="145"/>
      <source>length:</source>
      <translation>uzunluk:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="132"/>
      <source>Two elements are needed.</source>
      <translation>İki unsura ihtiyaç var.</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="139"/>
      <source>Radius is too large</source>
      <translation>Yarıçap çok büyük</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="143"/>
      <location filename="../../draftmake/make_fillet.py" line="144"/>
      <location filename="../../draftmake/make_fillet.py" line="145"/>
      <source>Segment</source>
      <translation>Bölüm</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="165"/>
      <source>Removed original objects.</source>
      <translation>Orijinal nesneler kaldırıldı.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="87"/>
      <source>Select an object to scale</source>
      <translation>Boyutlandırmak için bir nesne seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="108"/>
      <source>Pick base point</source>
      <translation>Temel noktasını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="135"/>
      <source>Pick reference distance from base point</source>
      <translation>Temel noktasından referans mesafesini seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="206"/>
      <location filename="../../draftguitools/gui_scale.py" line="236"/>
      <location filename="../../draftguitools/gui_scale.py" line="359"/>
      <source>Scale</source>
      <translation>Ölçek</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="209"/>
      <source>Some subelements could not be scaled.</source>
      <translation>Bazı alt öğeler ölçeklenemedi.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="339"/>
      <source>Unable to scale object:</source>
      <translation>Nesne ölçeklendirilemiyor:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="343"/>
      <source>Unable to scale objects:</source>
      <translation>Nesneler ölçeklendirilemiyor:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="346"/>
      <source>This object type cannot be scaled directly. Please use the clone method.</source>
      <translation>Bu nesne türü doğrudan ölçeklenemez. Lütfen klon yöntemini kullanın.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="407"/>
      <source>Pick new distance from base point</source>
      <translation>Temel noktasından yeni mesafeyi seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_sketcher_objects.py" line="63"/>
      <source>Sketch is too complex to edit: it is suggested to use sketcher default editor</source>
      <translation>Eskiz düzenlenemeyecek kadar karmaşık: Eskiz varsayılan düzenleyicisinin kullanılması önerilir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="80"/>
      <source>Pick target point</source>
      <translation>Hedef Noktası seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="157"/>
      <source>Create Label</source>
      <translation>Etiket Oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="191"/>
      <location filename="../../draftguitools/gui_labels.py" line="218"/>
      <source>Pick endpoint of leader line</source>
      <translation>Kılavuz çizginin bitiş noktasını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="201"/>
      <location filename="../../draftguitools/gui_labels.py" line="228"/>
      <source>Pick text position</source>
      <translation>Metin konumunu seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="75"/>
      <source>Change Style</source>
      <translation>Biçimi değiştir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="77"/>
      <source>Pick location point</source>
      <translation>Konum noktasını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="121"/>
      <source>Create Text</source>
      <translation>Metin oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="51"/>
      <source>Toggle grid</source>
      <translation>Izgarayı Aç/Kapa</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="150"/>
      <source>Pick a face, 3 vertices or a WP Proxy to define the drawing plane</source>
      <translation>Çizim düzlemini tanımlamak için bir yüzey, 3 nokta veya bir WP Proxy seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="274"/>
      <source>Working plane aligned to global placement of</source>
      <translation>Global yerleşime hizalanmış çalışma düzlemi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="523"/>
      <source>Dir</source>
      <translation>Yön</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="539"/>
      <source>Custom</source>
      <translation>Özel</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="71"/>
      <source>Select faces from existing objects</source>
      <translation>Mevcut nesnelerden yüzeyleri seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="73"/>
      <source>Select an object to mirror</source>
      <translation>Aynalamak için bir nesne seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="92"/>
      <source>Pick start point of mirror line</source>
      <translation>Aynalama hattının başlangıç noktasını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="122"/>
      <source>Mirror</source>
      <translation>Aynala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="177"/>
      <location filename="../../draftguitools/gui_mirror.py" line="203"/>
      <source>Pick end point of mirror line</source>
      <translation>Aynalama hattının bitim noktasını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="78"/>
      <location filename="../../draftguitools/gui_arcs.py" line="88"/>
      <source>Pick center point</source>
      <translation>Merkez noktasını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="189"/>
      <location filename="../../draftguitools/gui_polygons.py" line="200"/>
      <location filename="../../draftguitools/gui_polygons.py" line="260"/>
      <location filename="../../draftguitools/gui_arcs.py" line="254"/>
      <location filename="../../draftguitools/gui_arcs.py" line="270"/>
      <location filename="../../draftguitools/gui_arcs.py" line="410"/>
      <source>Pick radius</source>
      <translation>Yarıçapı seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="277"/>
      <location filename="../../draftguitools/gui_arcs.py" line="278"/>
      <location filename="../../draftguitools/gui_arcs.py" line="446"/>
      <location filename="../../draftguitools/gui_arcs.py" line="447"/>
      <source>Start angle</source>
      <translation>Başlangıç açısı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="283"/>
      <location filename="../../draftguitools/gui_arcs.py" line="452"/>
      <source>Pick start angle</source>
      <translation>Başlangıç açısını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="285"/>
      <location filename="../../draftguitools/gui_arcs.py" line="286"/>
      <location filename="../../draftguitools/gui_arcs.py" line="454"/>
      <location filename="../../draftguitools/gui_arcs.py" line="455"/>
      <source>Aperture angle</source>
      <translation>Aralık açısı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="291"/>
      <source>Pick aperture</source>
      <translation>Aralığı seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="317"/>
      <source>Create Circle (Part)</source>
      <translation>Daire Oluştur (Parça)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="335"/>
      <source>Create Circle</source>
      <translation>Çember oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="369"/>
      <source>Create Arc (Part)</source>
      <translation>Yay Oluştur (Parça)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="389"/>
      <source>Create Arc</source>
      <translation>Yay oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="466"/>
      <source>Pick aperture angle</source>
      <translation>Aralık açısını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="509"/>
      <location filename="../../draftguitools/gui_arcs.py" line="551"/>
      <source>Arc by 3 points</source>
      <translation>3 Noktalı Yay</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="71"/>
      <location filename="../../draftguitools/gui_lines.py" line="83"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="122"/>
      <location filename="../../draftguitools/gui_rectangles.py" line="69"/>
      <source>Pick first point</source>
      <translation>İlk noktayı seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="163"/>
      <source>Create Line</source>
      <translation>Çizgi oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="185"/>
      <source>Create Wire</source>
      <translation>Tel oluştur</translation>
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
      <translation>Bir sonraki noktayı seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="330"/>
      <source>Unable to create a Wire from selected objects</source>
      <translation>Seçilen nesnelerden bir Tel oluşturulamıyor</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="352"/>
      <source>Convert to Wire</source>
      <translation>Tel'e dönüştür</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="85"/>
      <location filename="../../draftguitools/gui_shapestrings.py" line="98"/>
      <source>Pick ShapeString location point</source>
      <translation>ŞekilDizesi konum noktası seçin</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="176"/>
      <location filename="../../draftguitools/gui_shapestrings.py" line="133"/>
      <source>Create ShapeString</source>
      <translation>ShapeString oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="305"/>
      <source>Select a Draft object to edit</source>
      <translation>Düzenlemek için bir taslak nesnesi seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="558"/>
      <source>No edit point found for selected object</source>
      <translation>Seçilen nesne için düzenleme noktası bulunamadı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="811"/>
      <source>Too many objects selected, max number set to:</source>
      <translation>Çok fazla nesne seçili, maksimum sayı şuna ayarlanıyor:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="819"/>
      <source>: this object is not editable</source>
      <translation>: bu nesne düzenlenebilir değil</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="76"/>
      <source>Select an object to join</source>
      <translation>Birleştirmek için bir nesne seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="99"/>
      <source>Join lines</source>
      <translation>Çizgileri Birleştir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="110"/>
      <source>Selection:</source>
      <translation>Seçim:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="64"/>
      <source>Change slope</source>
      <translation>Eğimi değiştir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="94"/>
      <source>Select objects to trim or extend</source>
      <translation>Kırpılacak veya uzatılacak nesneleri seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="173"/>
      <location filename="../../draftguitools/gui_offset.py" line="143"/>
      <source>Pick distance</source>
      <translation>Mesafe seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="222"/>
      <source>Offset angle</source>
      <translation>Öteleme açısı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="483"/>
      <source>Unable to trim these objects, only Draft wires and arcs are supported.</source>
      <translation>Bu nesneler kırpılamaz, yalnızca Taslak teller ve yaylar desteklenir.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="488"/>
      <source>Unable to trim these objects, too many wires</source>
      <translation>Bu nesneler kırplılamıyor, teller çok fazla</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="505"/>
      <source>These objects don't intersect.</source>
      <translation>Bu nesneler kesişmiyor.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="508"/>
      <source>Too many intersection points.</source>
      <translation>Çok fazla çakışma noktası.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="66"/>
      <source>Select an object to convert.</source>
      <translation>Dönüştürmek için bir nesne seçin.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="99"/>
      <source>Convert to Sketch</source>
      <translation>Eskize Dönüştür</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="114"/>
      <source>Convert to Draft</source>
      <translation>Taslağa dönüştür</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="143"/>
      <source>Convert Draft/Sketch</source>
      <translation>Taslak/Eskiz Dönüşümü</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="104"/>
      <source>Please select exactly two objects, the base object and the point object, before calling this command.</source>
      <translation>Lütfen bu komutu çağırmadan önce tam olarak iki nesne, temel nesne ve nokta nesnesi seçin.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="122"/>
      <source>Point array</source>
      <translation>Noktalara göre dizi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="108"/>
      <source>Select an object to edit</source>
      <translation>Düzenlenecek nesneyi seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="79"/>
      <source>Select an object to clone</source>
      <translation>Klonlamak için bir nesne seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="212"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="247"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="261"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="294"/>
      <source>Create Dimension</source>
      <translation>Ölçü Oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="309"/>
      <source>Create Dimension (radial)</source>
      <translation>Ölçü Oluştur (radyal)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="508"/>
      <source>Edge too short!</source>
      <translation>Kenar çok kısa!</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="518"/>
      <source>Edges don't intersect!</source>
      <translation>Kenarlar kesişmiyor!</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="75"/>
      <source>Select an object to stretch</source>
      <translation>Uzatmak için bir nesne seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="127"/>
      <source>Pick first point of selection rectangle</source>
      <translation>Seçim dikdörtgeninin ilk noktasını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="164"/>
      <source>Pick opposite point of selection rectangle</source>
      <translation>Seçim dikdörtgeninin karşıt noktasını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="173"/>
      <source>Pick start point of displacement</source>
      <translation>Yer değiştirme başlangıç noktasını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="236"/>
      <source>Pick end point of displacement</source>
      <translation>Yer değiştirme bitiş noktasını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="448"/>
      <source>Turning one Rectangle into a Wire</source>
      <translation>Dikdörtgeni bir Tele Dönüştür</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="477"/>
      <source>Stretch</source>
      <translation>Uzat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="102"/>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="79"/>
      <source>Please select exactly two objects, the base object and the path object, before calling this command.</source>
      <translation>Lütfen bu komutu çağırmadan önce tam olarak iki nesne, temel nesne ve yol nesnesi seçin.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="101"/>
      <source>Path twisted array</source>
      <translation>Yol bükülmüş dizi</translation>
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
      <translation>Son nokta kaldırıldı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="153"/>
      <location filename="../../draftguitools/gui_splines.py" line="147"/>
      <source>Pick next point, or finish (A) or close (O)</source>
      <translation>Sonraki nokta seçin veya Sonlandırın (A) ya da kapatın (O)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="211"/>
      <location filename="../../draftguitools/gui_beziers.py" line="451"/>
      <source>Create BezCurve</source>
      <translation>BezCurve oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="376"/>
      <source>Click and drag to define next knot</source>
      <translation>Sonraki düğümü tanımlamak için tıklayın ve sürükleyin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="382"/>
      <source>Click and drag to define next knot, or finish (A) or close (O)</source>
      <translation>Sonraki düğümü tanımlamak için tıklayın ve sürükleyin, ya da sonlandırın (A) veya kapatın (O)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snapper.py" line="1543"/>
      <source>(ON)</source>
      <translation>(Açık)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snapper.py" line="1546"/>
      <source>(OFF)</source>
      <translation>(Kapalı)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="67"/>
      <location filename="../../draftguitools/gui_upgrade.py" line="67"/>
      <source>Select an object to upgrade</source>
      <translation>Yükseltmek için bir nesne seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="85"/>
      <source>Downgrade</source>
      <translation>İndirge</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="143"/>
      <source>Path array</source>
      <translation>Nesneyi bir yol dizisine göre kopyalar</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="120"/>
      <source>Spline has been closed</source>
      <translation>Eğri kapatıldı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="183"/>
      <source>Create B-spline</source>
      <translation>B-spline'ı yarat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="125"/>
      <source>Create Plane</source>
      <translation>Düzlem oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="142"/>
      <source>Create Rectangle</source>
      <translation>Dikdörtgen oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="196"/>
      <location filename="../../draftguitools/gui_rectangles.py" line="202"/>
      <source>Pick opposite point</source>
      <translation>Karşıt noktayı seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="76"/>
      <source>Fillet radius</source>
      <translation>Kavis yarıçapı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="79"/>
      <source>Radius of fillet</source>
      <translation>Kavisin yarıçapı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="107"/>
      <source>Enter radius.</source>
      <translation>Yarıçapı girin.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="126"/>
      <source>Delete original objects:</source>
      <translation>Orijinal nesneleri silin:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="131"/>
      <source>Chamfer mode:</source>
      <translation>Pah modu:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="148"/>
      <source>Two elements needed.</source>
      <translation>İki unsur gerekli.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="155"/>
      <source>Test object</source>
      <translation>Test nesnesi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="156"/>
      <source>Test object removed</source>
      <translation>Test nesnesi kaldırıldı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="158"/>
      <source>Fillet cannot be created</source>
      <translation>Kavis oluşturulamaz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="188"/>
      <source>Create fillet</source>
      <translation>Fileto oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="65"/>
      <source>Add to group</source>
      <translation>Gruba ekle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="68"/>
      <source>Ungroup</source>
      <translation>Grubu çöz</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="70"/>
      <source>Add new group</source>
      <translation>Yeni grup ekle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="159"/>
      <source>Select group</source>
      <translation>Grubu seç</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="193"/>
      <source>No new selection. You must select non-empty groups or objects inside groups.</source>
      <translation>Yeni seçim yok. Boş olmayan gruplar veya grupların içindeki nesneleri seçmelisiniz.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="203"/>
      <source>Autogroup</source>
      <translation>Otogrup</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="250"/>
      <source>Add new Layer</source>
      <translation>Yeni Katman Ekle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="304"/>
      <source>Add to construction group</source>
      <translation>Yapı grubuna ekle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="355"/>
      <source>Add a new group with a given name</source>
      <translation>Verilen isimle yeni bir grup ekle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="383"/>
      <source>Add group</source>
      <translation>Grup ekle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="385"/>
      <source>Group name</source>
      <translation>Grup adı</translation>
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
      <translation>Bu nesne olası çakışma noktalarını desteklemiyor, lütfen tekrar deneyin.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="164"/>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="608"/>
      <source>Active object must have more than two points/nodes</source>
      <translation>Etkin nesnenin ikiden fazla noktası / düğümü olmalıdır</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="722"/>
      <source>Selection is not a Knot</source>
      <translation>Seçim, bir düğüm değil</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="749"/>
      <source>Endpoint of BezCurve can't be smoothed</source>
      <translation>BezCurve'un uç noktası yumuşatılamıyor</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="77"/>
      <source>The Drawing Workbench is obsolete since 0.17, consider using the TechDraw Workbench instead.</source>
      <translation>Çizim Tezgahı 0.17'den beri kullanılmıyor, bunun yerine TechDraw ÇalışmaTezgahı'nı kullanmayı düşünün.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="81"/>
      <location filename="../../draftguitools/gui_shape2dview.py" line="68"/>
      <source>Select an object to project</source>
      <translation>Yansıtılacak bir nesne seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="85"/>
      <source>Upgrade</source>
      <translation>Yükselt</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="126"/>
      <source>Main toggle snap</source>
      <translation>Ana Yakalamayı Ac/Kapat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="157"/>
      <source>Midpoint snap</source>
      <translation>OrtaNoktayı yakala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="187"/>
      <source>Perpendicular snap</source>
      <translation>Dik Yakala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="217"/>
      <source>Grid snap</source>
      <translation>Izgarayı Yakala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="247"/>
      <source>Intersection snap</source>
      <translation>Kesişim Yakala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="277"/>
      <source>Parallel snap</source>
      <translation>Paralel yakala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="307"/>
      <source>Endpoint snap</source>
      <translation>Uç noktayı yakala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="338"/>
      <source>Angle snap (30 and 45 degrees)</source>
      <translation>Açı Yakala (30 ve 45 dereceler)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="368"/>
      <source>Arc center snap</source>
      <translation>Yay merkezini yakala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="398"/>
      <source>Edge extension snap</source>
      <translation>Kenar uzantısını yakala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="428"/>
      <source>Near snap</source>
      <translation>Yakını yakala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="459"/>
      <source>Orthogonal snap</source>
      <translation>Dikeyi yakala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="489"/>
      <source>Special point snap</source>
      <translation>Özel nokta yakala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="520"/>
      <source>Dimension display</source>
      <translation>Ölçü görüntüsü</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="553"/>
      <source>Working plane snap</source>
      <translation>Çalışma düzlemi yakala</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="583"/>
      <source>Show snap toolbar</source>
      <translation>Yakalama Araç çubuğunda göster</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="81"/>
      <source>Select an object to move</source>
      <translation>Taşımak için bir nesneyi seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="103"/>
      <source>Pick start point</source>
      <translation>Başlangıç noktasını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="162"/>
      <location filename="../../draftguitools/gui_move.py" line="308"/>
      <source>Pick end point</source>
      <translation>Bitiş noktasını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="210"/>
      <source>Move</source>
      <translation>Taşı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="213"/>
      <location filename="../../draftguitools/gui_rotate.py" line="289"/>
      <source>Some subelements could not be moved.</source>
      <translation>Bazı alt öğeler taşınamadı.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="121"/>
      <location filename="../../draftguitools/gui_ellipses.py" line="138"/>
      <source>Create Ellipse</source>
      <translation>Elips Oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="55"/>
      <source>Flip dimension</source>
      <translation>Ölçüyü ters çevir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="73"/>
      <source>No active Draft Toolbar.</source>
      <translation>Etkin Taslak Araç Çubuğu yok.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="93"/>
      <source>Construction mode</source>
      <translation>Yapı Modu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="125"/>
      <source>Continue mode</source>
      <translation>Devam Modu</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="159"/>
      <source>Toggle display mode</source>
      <translation>Ekran modunu aç / kapat</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="68"/>
      <source>Annotation style editor</source>
      <translation>Açıklama Biçim Düzenleyicisi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="290"/>
      <source>Open styles file</source>
      <translation>Biçim dosyasını aç</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="292"/>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="314"/>
      <source>JSON file (*.json)</source>
      <translation>JSON dosyası (*.json)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="312"/>
      <source>Save styles file</source>
      <translation>Biçim dosyasını kaydet</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="51"/>
      <source>Heal</source>
      <translation>İyileştir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="134"/>
      <location filename="../../draftguitools/gui_points.py" line="147"/>
      <source>Create Point</source>
      <translation>Nokta Oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="224"/>
      <source>Create Polygon (Part)</source>
      <translation>Çokgen oluştur (Parça)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="243"/>
      <source>Create Polygon</source>
      <translation>Çokgen oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="76"/>
      <source>Select an object to offset</source>
      <translation>Ötelemek için bir nesneyi seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="82"/>
      <source>Offset only works on one object at a time.</source>
      <translation>Öteleme, bir seferde yalnızca bir nesne üzerinde çalışır.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="92"/>
      <source>Cannot offset this object type</source>
      <translation>Bu nesne türünü ötelenemez</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="123"/>
      <source>Offset of Bezier curves is currently not supported</source>
      <translation>Bezier eğrilerinin ötelenmesi şu anda desteklenmiyor</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="79"/>
      <source>Select an object to rotate</source>
      <translation>Döndürmek için bir nesne seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="99"/>
      <source>Pick rotation center</source>
      <translation>Döndürme merkezini seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="193"/>
      <location filename="../../draftguitools/gui_rotate.py" line="396"/>
      <source>Base angle</source>
      <translation>Taban açısı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="194"/>
      <location filename="../../draftguitools/gui_rotate.py" line="397"/>
      <source>The base angle you wish to start the rotation from</source>
      <translation>Dönüşü başlatmak istediğiniz taban açısı</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="199"/>
      <location filename="../../draftguitools/gui_rotate.py" line="400"/>
      <source>Pick base angle</source>
      <translation>Taban açısını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="205"/>
      <location filename="../../draftguitools/gui_rotate.py" line="409"/>
      <source>Rotation</source>
      <translation>Dönüş</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="206"/>
      <location filename="../../draftguitools/gui_rotate.py" line="410"/>
      <source>The amount of rotation you wish to perform.
The final angle will be the base angle plus this amount.</source>
      <translation>Gerçekleştirmek istediğiniz dönüş miktarı.
Son açı, taban açısı artı bu miktar olacaktır.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="213"/>
      <location filename="../../draftguitools/gui_rotate.py" line="418"/>
      <source>Pick rotation angle</source>
      <translation>Döndürme açısını seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="109"/>
      <source>Create 2D view</source>
      <translation>Yeni 2D görünüm oluştur</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="77"/>
      <source>Select an object to array</source>
      <translation>Sıralamak için bir nesne seçin</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="101"/>
      <source>Array</source>
      <translation>Dizi</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="64"/>
      <source>Click anywhere on a line to split it.</source>
      <translation>Bölmek için çizginin herhangi bir yerine tıklayın.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="106"/>
      <source>Split line</source>
      <translation>Çizgiyi böl</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="83"/>
      <source>Task panel:</source>
      <translation>Görev paneli:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="187"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="208"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="201"/>
      <source>At least one element must be selected.</source>
      <translation>En az bir öğe seçilmelidir.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="205"/>
      <source>Number of elements must be at least 1.</source>
      <translation>Eleman sayısı en az 1 olmalıdır.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="194"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="219"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="212"/>
      <source>Selection is not suitable for array.</source>
      <translation>Dizilim için seçim uygun değil.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="195"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="327"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="220"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="372"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="213"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="375"/>
      <source>Object:</source>
      <translation>Nesne:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="316"/>
      <source>Interval X reset:</source>
      <translation>X aralığını sıfırla:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="325"/>
      <source>Interval Y reset:</source>
      <translation>Y aralığını sıfırla:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="334"/>
      <source>Interval Z reset:</source>
      <translation>Z aralığını sıfırla:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="296"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="341"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="344"/>
      <source>Fuse:</source>
      <translation>Birleştir:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="310"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="355"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="358"/>
      <source>Create Link array:</source>
      <translation>Bağlantı dizisi oluşturun:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="376"/>
      <source>Number of X elements:</source>
      <translation>X öğelerinin sayısı:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="378"/>
      <source>Interval X:</source>
      <translation>X Aralığı:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="381"/>
      <source>Number of Y elements:</source>
      <translation>Y öğelerinin sayısı:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="383"/>
      <source>Interval Y:</source>
      <translation>Y Aralığı:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="386"/>
      <source>Number of Z elements:</source>
      <translation>Z öğelerinin sayısı:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="388"/>
      <source>Interval Z:</source>
      <translation>Z Aralığı:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="434"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="481"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="396"/>
      <source>Aborted:</source>
      <translation>İptal edildi:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="212"/>
      <source>Number of layers must be at least 2.</source>
      <translation>Katman sayısı en az 2 olmalıdır.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="224"/>
      <source>Radial distance is zero. Resulting array may not look correct.</source>
      <translation>Dairesel mesafe sıfırdır. Ortaya çıkan dizi doğru görünmeyebilir.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="226"/>
      <source>Radial distance is negative. It is made positive to proceed.</source>
      <translation>Dairesel mesafe negatif. Devam etmek için pozitif yapılır.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="230"/>
      <source>Tangential distance cannot be zero.</source>
      <translation>Teğetsel mesafe sıfır olamaz.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="233"/>
      <source>Tangential distance is negative. It is made positive to proceed.</source>
      <translation>Teğet uzaklık negatiftir. Devam etmek için pozitif yapılır.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="286"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="331"/>
      <source>Center reset:</source>
      <translation>Merkez sıfırlama:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="373"/>
      <source>Radial distance:</source>
      <translation>Dairesel mesafe:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="374"/>
      <source>Tangential distance:</source>
      <translation>Teğetsel mesafe:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="375"/>
      <source>Number of circular layers:</source>
      <translation>Dairesel katmanların sayısı:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="376"/>
      <source>Symmetry parameter:</source>
      <translation>Simetri parametresi:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="331"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="378"/>
      <source>Center of rotation:</source>
      <translation>Döndürme merkezi:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="199"/>
      <source>Number of elements must be at least 2.</source>
      <translation>Eleman sayısı en az 2 olmalıdır.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="203"/>
      <source>The angle is above 360 degrees. It is set to this value to proceed.</source>
      <translation>Açı 360 derecenin üzerinde. Devam etmek için bu değere ayarlanır.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="206"/>
      <source>The angle is below -360 degrees. It is set to this value to proceed.</source>
      <translation>Açı -360 derecenin altında. Devam etmek için bu değere ayarlanır.</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="328"/>
      <source>Number of elements:</source>
      <translation>Unsur sayısı:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="329"/>
      <source>Polar angle:</source>
      <translation>Kutupsal açı:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="56"/>
      <source>ShapeString</source>
      <translation>Şekil dizesi</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="70"/>
      <source>Default</source>
      <translation>Varsayılan</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="361"/>
      <source>Activate this layer</source>
      <translation>Bu katmanı etkinleştirin</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="367"/>
      <source>Select layer contents</source>
      <translation>Katman içeriklerini seçin</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="405"/>
      <location filename="../../draftviewproviders/view_layer.py" line="421"/>
      <source>Merge layer duplicates</source>
      <translation>Katman kopyalarını birleştir</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="410"/>
      <location filename="../../draftviewproviders/view_layer.py" line="469"/>
      <source>Add new layer</source>
      <translation>Yeni Katman Ekle</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="454"/>
      <source>Relabeling layer:</source>
      <translation>1:</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="458"/>
      <source>Merging layer:</source>
      <translation>Merging layer:</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_base.py" line="402"/>
      <source>Please load the Draft Workbench to enable editing this object</source>
      <translation>Bu nesneyi düzenlemeyi etkinleştirmek için lütfen Draft Çalışma Tezgahını yükleyin</translation>
    </message>
  </context>
  <context>
    <name>importOCA</name>
    <message>
      <location filename="../../importOCA.py" line="360"/>
      <source>OCA error: couldn't determine character encoding</source>
      <translation>OCA hatası: karakter kodlaması belirlenemedi</translation>
    </message>
    <message>
      <location filename="../../importOCA.py" line="445"/>
      <source>OCA: found no data to export</source>
      <translation>OCA: dışa aktarmak için bir veri bulamadı</translation>
    </message>
    <message>
      <location filename="../../importOCA.py" line="490"/>
      <source>successfully exported</source>
      <translation>başarıyla dışa aktarıldı</translation>
    </message>
  </context>
</TS>
