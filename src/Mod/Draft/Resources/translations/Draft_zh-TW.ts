<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_TW" sourcelanguage="en_US">
    <extra-po-header-language>zh_TW</extra-po-header-language>
    <extra-po-header-language_team></extra-po-header-language_team>
    <extra-po-header-last_translator>DrBenson &lt;Benson.Dr@GMail.com&gt;</extra-po-header-last_translator>
    <extra-po-header-po_revision_date></extra-po-header-po_revision_date>
    <extra-po-header-pot_creation_date></extra-po-header-pot_creation_date>
    <extra-po-header-project_id_version>FreeCAD Draft Language Translations v0.21_pre</extra-po-header-project_id_version>
    <extra-po-header-x_generator>Poedit 3.2.2</extra-po-header-x_generator>
    <extra-po-header_comment>#
# Translators:
# DrBenson &lt;Benson.Dr@GMail.com&gt;, 2023
#
    </extra-po-header_comment>
    <extra-po-headers>Project-Id-Version,POT-Creation-Date,PO-Revision-Date,Last-Translator,Language-Team,Language,MIME-Version,Content-Type,Content-Transfer-Encoding,Plural-Forms,X-Language,X-Source-Language,X-Qt-Contexts,X-Generator</extra-po-headers>
<context>
    <name>App::Property</name>
    <message>
        <location filename="../../draftobjects/wire.py" line="49"/>
        <source>The vertices of the wire</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/wire.py" line="53"/>
        <source>If the wire is closed or not</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/wire.py" line="57"/>
        <source>The base object is the wire, it&apos;s formed from 2 objects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/wire.py" line="61"/>
        <source>The tool object is the wire, it&apos;s formed from 2 objects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/wire.py" line="65"/>
        <source>The start point of this line</source>
        <translation>此線段之起點</translation>
    </message>
    <message>
        <location filename="../../draftobjects/wire.py" line="69"/>
        <source>The end point of this line</source>
        <translation>此線段之終點</translation>
    </message>
    <message>
        <location filename="../../draftobjects/wire.py" line="73"/>
        <source>The length of this line</source>
        <translation>此線段的長度</translation>
    </message>
    <message>
        <location filename="../../draftobjects/wire.py" line="85"/>
        <source>Create a face if this object is closed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/wire.py" line="89"/>
        <source>The number of subdivisions of each edge</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/rectangle.py" line="46"/>
        <source>Length of the rectangle</source>
        <translation>矩形的長度</translation>
    </message>
    <message>
        <location filename="../../draftobjects/rectangle.py" line="49"/>
        <source>Height of the rectangle</source>
        <translation>矩形的高度</translation>
    </message>
    <message>
        <location filename="../../draftobjects/rectangle.py" line="52"/>
        <location filename="../../draftobjects/polygon.py" line="60"/>
        <location filename="../../draftobjects/wire.py" line="77"/>
        <source>Radius to use to fillet the corners</source>
        <translation>倒角半徑</translation>
    </message>
    <message>
        <location filename="../../draftobjects/rectangle.py" line="55"/>
        <location filename="../../draftobjects/polygon.py" line="64"/>
        <location filename="../../draftobjects/wire.py" line="81"/>
        <source>Size of the chamfer to give to the corners</source>
        <translation>倒角尺寸</translation>
    </message>
    <message>
        <location filename="../../draftobjects/rectangle.py" line="58"/>
        <location filename="../../draftobjects/circle.py" line="58"/>
        <location filename="../../draftobjects/polygon.py" line="68"/>
        <location filename="../../draftobjects/ellipse.py" line="58"/>
        <source>Create a face</source>
        <translation>建立一個面</translation>
    </message>
    <message>
        <location filename="../../draftobjects/rectangle.py" line="61"/>
        <source>Horizontal subdivisions of this rectangle</source>
        <translation>此矩形的水平細分</translation>
    </message>
    <message>
        <location filename="../../draftobjects/rectangle.py" line="64"/>
        <source>Vertical subdivisions of this rectangle</source>
        <translation>此矩形的垂直細分</translation>
    </message>
    <message>
        <location filename="../../draftobjects/rectangle.py" line="67"/>
        <location filename="../../draftobjects/circle.py" line="62"/>
        <location filename="../../draftobjects/polygon.py" line="72"/>
        <location filename="../../draftobjects/bspline.py" line="57"/>
        <location filename="../../draftobjects/bezcurve.py" line="70"/>
        <location filename="../../draftobjects/wire.py" line="93"/>
        <source>The area of this object</source>
        <translation>此物體的面積</translation>
    </message>
    <message>
        <location filename="../../draftobjects/label.py" line="65"/>
        <source>The position of the tip of the leader line.
This point can be decorated with an arrow or another symbol.</source>
        <translation>引導線尖端的位置。
這個點可以設定為箭頭或其他符號。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/label.py" line="79"/>
        <source>Object, and optionally subelement, whose properties will be displayed
as &apos;Text&apos;, depending on &apos;Label Type&apos;.

&apos;Target&apos; won&apos;t be used if &apos;Label Type&apos; is set to &apos;Custom&apos;.</source>
        <translation>物體和可選的子元素，其屬性將顯示為「文字」，具體取決於「標籤類型」。

如果「標籤類型」設定為「自定義」，則不會使用「目標」。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/label.py" line="109"/>
        <source>The list of points defining the leader line; normally a list of three points.

The first point should be the position of the text, that is, the &apos;Placement&apos;,
and the last point should be the tip of the line, that is, the &apos;Target Point&apos;.
The middle point is calculated automatically depending on the chosen
&apos;Straight Direction&apos; and the &apos;Straight Distance&apos; value and sign.

If &apos;Straight Direction&apos; is set to &apos;Custom&apos;, the &apos;Points&apos; property
can be set as a list of arbitrary points.</source>
        <translation>定義引導線的點列表； 通常是三點列表。

第一個點為文字的位置，也就是「位置」，
最後一點為線的尖端，即「目標點」。
中點將根據選擇的「直線方向」與「直線距離」值和符號自動計算。

如果「直線方向」設定為「自定義」，則「點」屬性可以設定為任意點的列表。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/label.py" line="124"/>
        <source>The direction of the straight segment of the leader line.

If &apos;Custom&apos; is chosen, the points of the leader can be specified by
assigning a custom list to the &apos;Points&apos; attribute.</source>
        <translation>引導線直線段的方向。

如果選擇「自定義」，則可以透過將自定義列表設定「點」屬性來指定引導點。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/label.py" line="142"/>
        <source>The length of the straight segment of the leader line.

This is an oriented distance; if it is negative, the line will be drawn
to the left or below the &apos;Text&apos;, otherwise to the right or above it,
depending on the value of &apos;Straight Direction&apos;.</source>
        <translation>引導線直線段的長度。

這是一個定向距離； 如果為負數，則線將繪製在「文字」的左側或下方，
否則將繪製在其右側或上方，具體取決於「直線方向」的值。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/label.py" line="156"/>
        <source>The placement of the &apos;Text&apos; element in 3D space</source>
        <translation>3D 空間中「文字」元素的位置</translation>
    </message>
    <message>
        <location filename="../../draftobjects/label.py" line="166"/>
        <source>The text to display when &apos;Label Type&apos; is set to &apos;Custom&apos;</source>
        <translation>當「標籤類型」設定為「自定義」時顯示的文字</translation>
    </message>
    <message>
        <location filename="../../draftobjects/label.py" line="181"/>
        <source>The text displayed by this label.

This property is read-only, as the final text depends on &apos;Label Type&apos;,
and the object defined in &apos;Target&apos;.
The &apos;Custom Text&apos; is displayed only if &apos;Label Type&apos; is set to &apos;Custom&apos;.</source>
        <translation>文字由這個標籤顯示。

這個屬性是唯讀的，就像是最終文字是依賴於“標籤類型”，
而該物體定義在“目標”。
&quot;自訂文字&quot;只有在當“標籤類型”被設定為“自定義”時顯示。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/label.py" line="216"/>
        <source>The type of information displayed by this label.

If &apos;Custom&apos; is chosen, the contents of &apos;Custom Text&apos; will be used.
For other types, the string will be calculated automatically from the object defined in &apos;Target&apos;.
&apos;Tag&apos; and &apos;Material&apos; only work for objects that have these properties, like Arch objects.

For &apos;Position&apos;, &apos;Length&apos;, and &apos;Area&apos; these properties will be extracted from the main object in &apos;Target&apos;,
or from the subelement &apos;VertexN&apos;, &apos;EdgeN&apos;, or &apos;FaceN&apos;, respectively, if it is specified.</source>
        <translation>這個標籤顯示了資訊的類型。

如果選擇了“自訂”，“自訂文字”的內容將會被使用。
至於其他類型，定義為“目標”的物體字串會自動被計算。
&quot;標籤&quot;和“材質”只適用擁有這些屬性的物體上，像是弧形物體。

如果被指定，像“位置”、“長度”和“面積”這些屬性會被從主要的”目標“物體抽取，或是分別從次元素”頂點Ｎ“、”邊Ｎ“、”面Ｎ“分別抽取。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/hatch.py" line="46"/>
        <source>The base object used by this object</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/hatch.py" line="49"/>
        <source>The PAT file used by this object</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/hatch.py" line="52"/>
        <source>The pattern name used by this object</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/hatch.py" line="55"/>
        <source>The pattern scale used by this object</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/hatch.py" line="58"/>
        <source>The pattern rotation used by this object</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/hatch.py" line="61"/>
        <source>If set to False, hatch is applied as is to the faces, without translation (this might give wrong results for non-XY faces)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/drawingview.py" line="66"/>
        <source>The linked object</source>
        <translation>連結的物體</translation>
    </message>
    <message>
        <location filename="../../draftobjects/drawingview.py" line="73"/>
        <source>Projection direction</source>
        <translation>投影方向</translation>
    </message>
    <message>
        <location filename="../../draftobjects/drawingview.py" line="80"/>
        <source>The width of the lines inside this object</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/drawingview.py" line="88"/>
        <source>The size of the texts inside this object</source>
        <translation>此物體內文字的大小</translation>
    </message>
    <message>
        <location filename="../../draftobjects/drawingview.py" line="96"/>
        <source>The spacing between lines of text</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/drawingview.py" line="103"/>
        <source>The color of the projected objects</source>
        <translation>被投影物體之顏色</translation>
    </message>
    <message>
        <location filename="../../draftobjects/drawingview.py" line="110"/>
        <source>Shape Fill Style</source>
        <translation>填充樣式</translation>
    </message>
    <message>
        <location filename="../../draftobjects/drawingview.py" line="118"/>
        <source>Line Style</source>
        <translation>線條樣式</translation>
    </message>
    <message>
        <location filename="../../draftobjects/drawingview.py" line="127"/>
        <source>If checked, source objects are displayed regardless of being visible in the 3D model</source>
        <translation>如果選中，則無論在 3D 模型中是否可見，都將顯示源物體</translation>
    </message>
    <message>
        <location filename="../../draftobjects/circle.py" line="45"/>
        <source>Start angle of the arc</source>
        <translation>這個弧度的起始角度</translation>
    </message>
    <message>
        <location filename="../../draftobjects/circle.py" line="50"/>
        <source>End angle of the arc (for a full circle, 
                give it same value as First Angle)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/circle.py" line="54"/>
        <source>Radius of the circle</source>
        <translation>圓的半徑</translation>
    </message>
    <message>
        <location filename="../../draftobjects/shapestring.py" line="46"/>
        <source>Text string</source>
        <translation>文字字串</translation>
    </message>
    <message>
        <location filename="../../draftobjects/shapestring.py" line="49"/>
        <source>Font file name</source>
        <translation>字型檔案名稱</translation>
    </message>
    <message>
        <location filename="../../draftobjects/shapestring.py" line="52"/>
        <source>Height of text</source>
        <translation>字高</translation>
    </message>
    <message>
        <location filename="../../draftobjects/shapestring.py" line="55"/>
        <source>Inter-character spacing</source>
        <translation>字元間距</translation>
    </message>
    <message>
        <location filename="../../draftobjects/shapestring.py" line="58"/>
        <source>Fill letters with faces</source>
        <translation>在面上填充字母</translation>
    </message>
    <message>
        <location filename="../../draftobjects/pathtwistedarray.py" line="86"/>
        <source>The base object that will be duplicated.</source>
        <translation>將被複製的基本物體。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/pathtwistedarray.py" line="93"/>
        <location filename="../../draftobjects/patharray.py" line="177"/>
        <source>The object along which the copies will be distributed. It must contain &apos;Edges&apos;.</source>
        <translation>將沿其分布複製的物體。 它必須包含「邊緣」。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/pathtwistedarray.py" line="100"/>
        <source>Number of copies to create.</source>
        <translation>欲複製的數量。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/pathtwistedarray.py" line="107"/>
        <source>Rotation factor of the twisted array.</source>
        <translation>扭曲陣列的旋轉係數。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="82"/>
        <location filename="../../draftobjects/patharray.py" line="169"/>
        <source>The base object that will be duplicated</source>
        <translation>將被複製的基本物體</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="98"/>
        <source>The type of array to create.
- Ortho: places the copies in the direction of the global X, Y, Z axes.
- Polar: places the copies along a circular arc, up to a specified angle, and with certain orientation defined by a center and an axis.
- Circular: places the copies in concentric circular layers around the base object.</source>
        <translation>要建立的陣列類型。
- Ortho：將複製放置於全域 X、Y、Z 軸的方向上。
- Polar：將複製沿圓弧放置，直到指定角度，並具有由中心和軸定義的特定方向。
- Circular：將複製放置於環繞基礎物體的同心圓層中。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="109"/>
        <source>Specifies if the copies should be fused together if they touch each other (slower)</source>
        <translation>如果複製相互接觸，是否對它們進行合併（較慢）</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="122"/>
        <source>Number of copies in X direction</source>
        <translation>X方向的複製數</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="131"/>
        <source>Number of copies in Y direction</source>
        <translation>Y方向的複製數</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="140"/>
        <source>Number of copies in Z direction</source>
        <translation>Z方向的複製數</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="150"/>
        <source>Distance and orientation of intervals in X direction</source>
        <translation>X方向區間的距離和方向</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="160"/>
        <source>Distance and orientation of intervals in Y direction</source>
        <translation>Y方向區間的距離和方向</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="170"/>
        <source>Distance and orientation of intervals in Z direction</source>
        <translation>Z方向區間的距離和方向</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="185"/>
        <source>The axis direction around which the elements in a polar or a circular array will be created</source>
        <translation>將建立極坐標或圓形陣列中元素所圍繞的軸方向</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="196"/>
        <source>Center point for polar and circular arrays.
The &apos;Axis&apos; passes through this point.</source>
        <translation>極座標與圓形陣列的中心點。
「軸」將穿過這一點。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="215"/>
        <source>The axis object that overrides the value of &apos;Axis&apos; and &apos;Center&apos;, for example, a datum line.
Its placement, position and rotation, will be used when creating polar and circular arrays.
Leave this property empty to be able to set &apos;Axis&apos; and &apos;Center&apos; manually.</source>
        <translation>覆蓋「軸」與「中心」值的軸物體，例如基準線。
它的位置、位置和旋轉將在建立極坐標和圓形陣列時使用。
將此屬性留空，以便能手動設定「軸」與「中心」。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="228"/>
        <source>Number of copies in the polar direction</source>
        <translation>極方向的複製數</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="238"/>
        <source>Distance and orientation of intervals in &apos;Axis&apos; direction</source>
        <translation>「軸」方向間隔的距離與方向</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="247"/>
        <source>Angle to cover with copies</source>
        <translation>覆蓋複製的角度</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="260"/>
        <source>Distance between circular layers</source>
        <translation>圓形圖層之間的距離</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="270"/>
        <source>Distance between copies in the same circular layer</source>
        <translation>同一圓形圖層中複製之間的距離</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="280"/>
        <source>Number of circular layers. The &apos;Base&apos; object counts as one layer.</source>
        <translation>圓形圖層的數量。「基礎」物體計為一層。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="291"/>
        <source>A parameter that determines how many symmetry planes the circular array will have.</source>
        <translation>確定圓形陣列具有多少個對稱平面的參數。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="309"/>
        <source>Total number of elements in the array.
This property is read-only, as the number depends on the parameters of the array.</source>
        <translation>陣列中的元素總數。
此屬性是唯讀的，因為數量取決於陣列的參數。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/array.py" line="320"/>
        <location filename="../../draftobjects/pathtwistedarray.py" line="114"/>
        <location filename="../../draftobjects/pointarray.py" line="112"/>
        <location filename="../../draftobjects/patharray.py" line="208"/>
        <source>Show the individual array elements (only for Link arrays)</source>
        <translation>顯示單個陣列元素（僅適用於連結陣列）</translation>
    </message>
    <message>
        <location filename="../../draftobjects/draft_annotation.py" line="83"/>
        <location filename="../../draftviewproviders/view_draft_annotation.py" line="82"/>
        <source>General scaling factor that affects the annotation consistently
because it scales the text, and the line decorations, if any,
in the same proportion.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/draft_annotation.py" line="93"/>
        <location filename="../../draftviewproviders/view_draft_annotation.py" line="100"/>
        <source>Annotation style to apply to this object.
When using a saved style some of the view properties will become read-only;
they will only be editable by changing the style through the &apos;Annotation style editor&apos; tool.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/draftlink.py" line="99"/>
        <source>Force sync pattern placements even when array elements are expanded</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/draftlink.py" line="112"/>
        <source>Show the individual array elements</source>
        <translation>顯示單獨的陣列元素</translation>
    </message>
    <message>
        <location filename="../../draftobjects/clone.py" line="46"/>
        <source>The objects included in this clone</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/clone.py" line="51"/>
        <source>The scale factor of this clone</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/clone.py" line="57"/>
        <source>If Clones includes several objects,
set True for fusion or False for compound</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/bspline.py" line="46"/>
        <source>The points of the B-spline</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/bspline.py" line="50"/>
        <source>If the B-spline is closed or not</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/bspline.py" line="54"/>
        <source>Create a face if this spline is closed</source>
        <translation>如果為封閉樣條，則建立一個面</translation>
    </message>
    <message>
        <location filename="../../draftobjects/bspline.py" line="67"/>
        <source>Parameterization factor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="57"/>
        <source>The base object this 2D view must represent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="62"/>
        <source>The projection vector of this object</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="68"/>
        <source>The way the viewed object must be projected</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="75"/>
        <source>The indices of the faces to be projected in Individual Faces mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="80"/>
        <source>Show hidden lines</source>
        <translation>顯示影藏線</translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="86"/>
        <source>Fuse wall and structure objects of same type and material</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="91"/>
        <source>Tessellate Ellipses and B-splines into line segments</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="98"/>
        <source>For Cutlines and Cutfaces modes, 
                    this leaves the faces at the cut location</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="105"/>
        <source>Length of line segments if tessellating Ellipses or B-splines 
                    into line segments</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="111"/>
        <source>If this is True, this object will include only visible objects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="117"/>
        <source>A list of exclusion points. Any edge touching any of those points will not be drawn.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="122"/>
        <source>If this is True, only solid geometry is handled. This overrides the base object&apos;s Only Solids property</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="127"/>
        <source>If this is True, the contents are clipped to the borders of the section plane, if applicable. This overrides the base object&apos;s Clip property</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shape2dview.py" line="132"/>
        <source>This object will be recomputed only if this is True.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/point.py" line="45"/>
        <source>X Location</source>
        <translation>X 位置</translation>
    </message>
    <message>
        <location filename="../../draftobjects/point.py" line="48"/>
        <source>Y Location</source>
        <translation>Y 位置</translation>
    </message>
    <message>
        <location filename="../../draftobjects/point.py" line="51"/>
        <source>Z Location</source>
        <translation>Z 位置</translation>
    </message>
    <message>
        <location filename="../../draftobjects/facebinder.py" line="43"/>
        <source>Linked faces</source>
        <translation>連結的面</translation>
    </message>
    <message>
        <location filename="../../draftobjects/facebinder.py" line="46"/>
        <source>Specifies if splitter lines must be removed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/facebinder.py" line="49"/>
        <source>An optional extrusion value to be applied to all faces</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/facebinder.py" line="52"/>
        <source>An optional offset value to be applied to all faces</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/facebinder.py" line="55"/>
        <source>This specifies if the shapes sew</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/facebinder.py" line="58"/>
        <source>The area of the faces of this Facebinder</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/polygon.py" line="48"/>
        <source>Number of faces</source>
        <translation>面的數量</translation>
    </message>
    <message>
        <location filename="../../draftobjects/polygon.py" line="52"/>
        <source>Radius of the control circle</source>
        <translation>控制圓的半徑</translation>
    </message>
    <message>
        <location filename="../../draftobjects/polygon.py" line="56"/>
        <source>How the polygon must be drawn from the control circle</source>
        <translation>如何從控制圓中繪製多邊形</translation>
    </message>
    <message>
        <location filename="../../draftobjects/block.py" line="43"/>
        <source>The components of this block</source>
        <translation>此區塊的元件</translation>
    </message>
    <message>
        <location filename="../../draftobjects/fillet.py" line="47"/>
        <source>The start point of this line.</source>
        <translation>此線段的起點。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/fillet.py" line="55"/>
        <source>The end point of this line.</source>
        <translation>此線段的終點。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/fillet.py" line="63"/>
        <source>The length of this line.</source>
        <translation>此線段的長度。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/fillet.py" line="71"/>
        <source>Radius to use to fillet the corner.</source>
        <translation>倒角的半徑。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/wpproxy.py" line="42"/>
        <source>The placement of this object</source>
        <translation>這個物體的位置</translation>
    </message>
    <message>
        <location filename="../../draftobjects/layer.py" line="59"/>
        <source>The objects that are part of this layer</source>
        <translation>物體是此圖層的一部分</translation>
    </message>
    <message>
        <location filename="../../draftobjects/dimension.py" line="137"/>
        <source>The normal direction of the text of the dimension</source>
        <translation>標示尺寸文字的法線方向</translation>
    </message>
    <message>
        <location filename="../../draftobjects/dimension.py" line="150"/>
        <source>The object measured by this dimension object</source>
        <translation>尺寸物體標示測量的物體</translation>
    </message>
    <message>
        <location filename="../../draftobjects/dimension.py" line="167"/>
        <source>The object, and specific subelements of it,
that this dimension object is measuring.

There are various possibilities:
- An object, and one of its edges.
- An object, and two of its vertices.
- An arc object, and its edge.</source>
        <translation>此物體尺寸標示正在測量的物體及其特定子元素。

有各種可能性：
- 物體及其邊緣之一。
- 物體及其兩個頂點。
- 弧形物體及其邊緣。</translation>
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
        <translation>一個在尺寸線上或在尺寸線的延長線上的點

- 對於線性尺寸標示，這個屬性控制尺寸線與被測物體之間的距離。
- 對於徑向尺寸標示，它控制顯示測量半徑或直徑的尺寸線的方向。
- 對於角度尺寸標示，它控制顯示測量角度的尺寸弧度的半徑。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/dimension.py" line="233"/>
        <source>Starting point of the dimension line.

If it is a radius dimension it will be the center of the arc.
If it is a diameter dimension it will be a point that lies on the arc.</source>
        <translation>尺寸線的起點。

如果為半徑尺寸標示，則是圓弧的中心點。
如果為直徑尺寸標示，則是圓弧上的一點。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/dimension.py" line="247"/>
        <source>Ending point of the dimension line.

If it is a radius or diameter dimension
it will be a point that lies on the arc.</source>
        <translation>尺寸線的終點。

如果是半徑或直徑標示
它將為圓弧上的一個點。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/dimension.py" line="259"/>
        <source>The direction of the dimension line.
If this remains &apos;(0,0,0)&apos;, the direction will be calculated automatically.</source>
        <translation>尺寸線的方向。
如果仍為「(0,0,0)」，將自動計算方向。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/dimension.py" line="276"/>
        <source>The value of the measurement.

This property is read-only because the value is calculated
from the &apos;Start&apos; and &apos;End&apos; properties.

If the &apos;Linked Geometry&apos; is an arc or circle, this &apos;Distance&apos;
is the radius or diameter, depending on the &apos;Diameter&apos; property.</source>
        <translation>測量值。

此屬性是唯讀的，因為該值是由「起點」與「終點」計算出來的。

如果「連結幾何」是圓弧或圓，則此「距離」為半徑或直徑，將取決於「直徑」屬性。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/dimension.py" line="287"/>
        <source>When measuring circular arcs, it determines whether to display
the radius or the diameter value</source>
        <translation>測量圓弧時，決定是否顯示半徑或直徑值</translation>
    </message>
    <message>
        <location filename="../../draftobjects/dimension.py" line="507"/>
        <source>Starting angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
        <translation>尺寸線（圓弧）的起始角度。
圓弧是逆時針方向繪製的。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/dimension.py" line="518"/>
        <source>Ending angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
        <translation>尺寸線（圓弧）的結束角度。
圓弧是逆時針方向繪製的。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/dimension.py" line="533"/>
        <source>The center point of the dimension line, which is a circular arc.

This is normally the point where two line segments, or their extensions
intersect, resulting in the measured &apos;Angle&apos; between them.</source>
        <translation>圓弧為尺寸線的中心點。

這通常是兩條線段或其延伸的點
相交，從而在它們之間產生測量的“角度”。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/dimension.py" line="547"/>
        <source>The value of the measurement.

This property is read-only because the value is calculated from
the &apos;First Angle&apos; and &apos;Last Angle&apos; properties.</source>
        <translation>測量值。

此屬性是唯讀的，因為該值是由「第一個角度」和「最後一個角度」屬性計算而來。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/ellipse.py" line="45"/>
        <source>Start angle of the elliptical arc</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/ellipse.py" line="49"/>
        <source>End angle of the elliptical arc 

                (for a full circle, give it same value as First Angle)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/ellipse.py" line="52"/>
        <source>Minor radius of the ellipse</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/ellipse.py" line="55"/>
        <source>Major radius of the ellipse</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/ellipse.py" line="61"/>
        <source>Area of this object</source>
        <translation>此物體的面積</translation>
    </message>
    <message>
        <location filename="../../draftobjects/text.py" line="54"/>
        <source>The placement of the base point of the first line</source>
        <translation>第一行基準點的位置</translation>
    </message>
    <message>
        <location filename="../../draftobjects/text.py" line="66"/>
        <source>The text displayed by this object.
It is a list of strings; each element in the list will be displayed in its own line.</source>
        <translation>此物體顯示的文字。
它是一個字串列表； 列表中的每個元素都將顯示在自己的行中。</translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="192"/>
        <source>List of connected edges in the &apos;Path Object&apos;.
If these are present, the copies will be created along these subelements only.
Leave this property empty to create copies along the entire &apos;Path Object&apos;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="200"/>
        <source>Number of copies to create</source>
        <translation>欲複製的數量</translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="219"/>
        <source>Additional translation that will be applied to each copy.
This is useful to adjust for the difference between shape centre and shape reference point.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="227"/>
        <source>Alignment vector for &apos;Tangent&apos; mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="235"/>
        <source>Force use of &apos;Vertical Vector&apos; as local Z direction when using &apos;Original&apos; or &apos;Tangent&apos; alignment mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="243"/>
        <source>Direction of the local Z axis when &apos;Force Vertical&apos; is true</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="251"/>
        <source>Method to orient the copies along the path.
- Original: X is curve tangent, Y is normal, and Z is the cross product.
- Frenet: aligns the object following the local coordinate system along the path.
- Tangent: similar to &apos;Original&apos; but the local X axis is pre-aligned to &apos;Tangent Vector&apos;.

To get better results with &apos;Original&apos; or &apos;Tangent&apos; you may have to set &apos;Force Vertical&apos; to true.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="262"/>
        <source>Orient the copies along the path depending on the &apos;Align Mode&apos;.
Otherwise the copies will have the same orientation as the original Base object.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/bezcurve.py" line="46"/>
        <source>The points of the Bezier curve</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/bezcurve.py" line="50"/>
        <source>The degree of the Bezier function</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/bezcurve.py" line="54"/>
        <source>Continuity</source>
        <translation>Continuity</translation>
    </message>
    <message>
        <location filename="../../draftobjects/bezcurve.py" line="58"/>
        <source>If the Bezier curve should be closed or not</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/bezcurve.py" line="62"/>
        <source>Create a face if this curve is closed</source>
        <translation>如果為封閉曲線，則建立一個面</translation>
    </message>
    <message>
        <location filename="../../draftobjects/bezcurve.py" line="66"/>
        <source>The length of this object</source>
        <translation>此物體的長度</translation>
    </message>
    <message>
        <location filename="../../draftobjects/pointarray.py" line="79"/>
        <source>Base object that will be duplicated</source>
        <translation>基本物體將被複製</translation>
    </message>
    <message>
        <location filename="../../draftobjects/pointarray.py" line="87"/>
        <source>Object containing points used to distribute the base object, for example, a sketch or a Part compound.
The sketch or compound must contain at least one explicit point or vertex object.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/pointarray.py" line="95"/>
        <source>Total number of elements in the array.
This property is read-only, as the number depends on the points contained within &apos;Point Object&apos;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/pointarray.py" line="104"/>
        <location filename="../../draftobjects/pointarray.py" line="140"/>
        <source>Additional placement, shift and rotation, that will be applied to each copy</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_text.py" line="60"/>
        <location filename="../../draftviewproviders/view_label.py" line="74"/>
        <source>The size of the text</source>
        <translation>字體大小</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_text.py" line="69"/>
        <location filename="../../draftviewproviders/view_label.py" line="83"/>
        <source>The font of the text</source>
        <translation>字型</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_text.py" line="78"/>
        <location filename="../../draftviewproviders/view_label.py" line="92"/>
        <location filename="../../draftviewproviders/view_label.py" line="119"/>
        <source>The vertical alignment of the text</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_text.py" line="87"/>
        <location filename="../../draftviewproviders/view_label.py" line="102"/>
        <source>Text color</source>
        <translation>文字顏色</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_text.py" line="95"/>
        <location filename="../../draftviewproviders/view_label.py" line="128"/>
        <source>Line spacing (relative to font size)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_label.py" line="111"/>
        <source>The maximum number of characters on each line of the text box</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_label.py" line="139"/>
        <source>The size of the arrow</source>
        <translation>箭頭尺寸</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_label.py" line="148"/>
        <source>The type of arrow of this label</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_label.py" line="159"/>
        <source>The type of frame around the text of this object</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_label.py" line="168"/>
        <source>Display a leader line or not</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_draft_annotation.py" line="115"/>
        <location filename="../../draftviewproviders/view_label.py" line="177"/>
        <source>Line width</source>
        <translation>線寬</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_draft_annotation.py" line="122"/>
        <location filename="../../draftviewproviders/view_label.py" line="186"/>
        <source>Line color</source>
        <translation>線條顏色</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="137"/>
        <source>Font name</source>
        <translation>字體名稱</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="146"/>
        <source>Font size</source>
        <translation>字體大小</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="155"/>
        <source>Spacing between text and dimension line</source>
        <translation>文字與尺寸線間的間距</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="164"/>
        <source>Rotate the dimension text 180 degrees</source>
        <translation>將尺寸文字旋轉180度</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="174"/>
        <source>Text Position.
Leave &apos;(0,0,0)&apos; for automatic position</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="185"/>
        <source>Text override.
Write &apos;$dim&apos; so that it is replaced by the dimension length.</source>
        <translation>文字覆蓋。
輸入「$dim」將其取代為尺寸長度。</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="196"/>
        <source>The number of decimals to show</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="205"/>
        <source>Show the unit suffix</source>
        <translation>在字尾顯示單位</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="216"/>
        <source>A unit to express the measurement.
Leave blank for system default.
Use &apos;arch&apos; to force US arch notation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="230"/>
        <source>Arrow size</source>
        <translation>箭頭尺寸</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="239"/>
        <source>Arrow type</source>
        <translation>箭頭樣式</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="249"/>
        <source>Rotate the dimension arrows 180 degrees</source>
        <translation>將尺寸箭頭旋轉180度</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="260"/>
        <source>The distance the dimension line is extended
past the extension lines</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="269"/>
        <source>Length of the extension lines</source>
        <translation>延伸線長度</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="279"/>
        <source>Length of the extension line
beyond the dimension line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_dimension.py" line="288"/>
        <source>Shows the dimension line and arrows</source>
        <translation>顯示尺寸線與箭頭</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="67"/>
        <source>If it is true, the objects contained within this layer will adopt the line color of the layer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="78"/>
        <source>If it is true, the objects contained within this layer will adopt the shape color of the layer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="89"/>
        <source>If it is true, the print color will be used when objects in this layer are placed on a TechDraw page</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="103"/>
        <source>The line color of the objects contained within this layer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="117"/>
        <source>The shape color of the objects contained within this layer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="131"/>
        <source>The line width of the objects contained within this layer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="143"/>
        <source>The draw style of the objects contained within this layer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="154"/>
        <source>The transparency of the objects contained within this layer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="165"/>
        <source>The line color of the objects contained within this layer, when used on a TechDraw page</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_base.py" line="106"/>
        <source>Defines an SVG pattern.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_base.py" line="116"/>
        <source>Defines the size of the SVG pattern.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Dialog</name>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="14"/>
        <source>Annotation Styles Editor</source>
        <translation>註解樣式編輯</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="20"/>
        <source>Style name</source>
        <translation>樣式名稱</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="38"/>
        <source>The name of your style. Existing style names can be edited.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="50"/>
        <source>Add new...</source>
        <translation>新增...</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="79"/>
        <source>Renames the selected style</source>
        <translation>重新命名選定的樣式</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="82"/>
        <source>Rename</source>
        <translation>重新命名</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="110"/>
        <source>Deletes the selected style</source>
        <translation>刪除選定的樣式</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="113"/>
        <source>Delete</source>
        <translation>刪除</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="120"/>
        <source>Import styles from json file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="130"/>
        <source>Export styles to json file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="164"/>
        <source>Text</source>
        <translation>文字</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="170"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="192"/>
        <source>The font to use for texts and dimensions</source>
        <translation>文字與尺寸使用的字體</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="173"/>
        <source>Font name</source>
        <translation>字體名稱</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="199"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="209"/>
        <source>Font size in the system units</source>
        <translation>系統單位文字的字體大小</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="219"/>
        <source>Line spacing in system units</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="331"/>
        <source>Line and arrows</source>
        <translation>尺寸線與箭頭</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="340"/>
        <source>Show lines</source>
        <translation>顯示線段</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="363"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="373"/>
        <source>The width of the dimension lines</source>
        <translation type="unfinished"></translation>
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
        <translation>尺寸線、箭頭和文字的顏色</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="389"/>
        <source>Line / text color</source>
        <translation>線條/文字顏色</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="410"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="432"/>
        <source>The type of arrows or markers to use at the end of dimension lines</source>
        <translation>於尺寸線尾端使用的箭頭或標記類型</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="464"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="474"/>
        <source>The size of the dimension arrows or markers in system units</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="484"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="494"/>
        <source>The distance that the dimension line is additionally extended</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="524"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="534"/>
        <source>The distance that the extension lines are additionally extended beyond the dimension line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="199"/>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="209"/>
        <source>The font size in system units</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="202"/>
        <source>Font size</source>
        <translation>字體大小</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="219"/>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="229"/>
        <source>The line spacing for multi-line texts and labels (relative to the font size)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="222"/>
        <source>Line spacing</source>
        <translation>行距</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="236"/>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="246"/>
        <source>The color of texts, dimension texts and label texts</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="239"/>
        <source>Text color</source>
        <translation>文字顏色</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="256"/>
        <source>Lines and arrows</source>
        <translation>線和箭頭</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="337"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="347"/>
        <source>If it is checked it will display the dimension line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="265"/>
        <source>Show line</source>
        <translation>顯示線</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="288"/>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="298"/>
        <source>The width of the lines</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="366"/>
        <source>Line width</source>
        <translation>線寬</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="301"/>
        <source> px</source>
        <translation> px</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="308"/>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="330"/>
        <source>The type of arrows or markers to use for dimensions and labels</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="413"/>
        <source>Arrow type</source>
        <translation>箭頭樣式</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="436"/>
        <source>Dot</source>
        <translation>點</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="441"/>
        <source>Circle</source>
        <translation>圓</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="446"/>
        <source>Arrow</source>
        <translation>箭頭</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="451"/>
        <source>Tick</source>
        <translation>鉤號</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="456"/>
        <source>Tick-2</source>
        <translation>鉤號-2</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="362"/>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="372"/>
        <source>The size of the arrows or markers in system units</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="467"/>
        <source>Arrow size</source>
        <translation>箭頭尺寸</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="382"/>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="392"/>
        <source>The color of lines and arrows</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="385"/>
        <source>Line and arrow color</source>
        <translation>線和箭頭顏色</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="239"/>
        <source>Units</source>
        <translation>單位</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="245"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="255"/>
        <source>A multiplier factor that affects the size of texts and markers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="248"/>
        <source>Scale multiplier</source>
        <translation>縮放比率</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="268"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="278"/>
        <source>If it is checked it will show the unit next to the dimension value</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="271"/>
        <source>Show unit</source>
        <translation>顯示單位</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="291"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="301"/>
        <source>Specify a valid length unit like mm, m, in, ft, to force displaying the dimension value in this unit</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="294"/>
        <source>Unit override</source>
        <translation>單位覆蓋</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="308"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="318"/>
        <source>The number of decimals to show for dimension values</source>
        <translation>尺寸值顯示的小數位數</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="311"/>
        <source>Decimals</source>
        <translation>小數位數</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="485"/>
        <source>Dimension details</source>
        <translation>尺寸細節</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="491"/>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="501"/>
        <source>The distance the dimension line is additionally extended</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="487"/>
        <source>Dimension overshoot</source>
        <translation>尺寸超越量</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="504"/>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="514"/>
        <source>The length of the extension lines</source>
        <translation>延伸線長度</translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="507"/>
        <source>Extension lines</source>
        <translation>延伸線</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="531"/>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="541"/>
        <source>The distance the extension lines are additionally extended beyond the dimension line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="527"/>
        <source>Extension overshoot</source>
        <translation>延伸超調</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="551"/>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="561"/>
        <source>The distance between the dimension text and the dimension line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialog_AnnotationStyleEditor.ui" line="554"/>
        <source>Text spacing</source>
        <translation>文字間距</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialogLayers.ui" line="14"/>
        <source>Layers manager</source>
        <translation>圖層管理</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialogLayers.ui" line="29"/>
        <source>New</source>
        <translation>新增</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialogLayers.ui" line="43"/>
        <source>Select all</source>
        <translation>全選</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialogLayers.ui" line="50"/>
        <source>Toggle on/off</source>
        <translation>切換 開啟/關閉</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialogLayers.ui" line="57"/>
        <source>Isolate</source>
        <translation>隔離</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialogLayers.ui" line="77"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialogLayers.ui" line="84"/>
        <source>OK</source>
        <translation>確定</translation>
    </message>
</context>
<context>
    <name>Draft</name>
    <message>
        <location filename="../../importDXF.py" line="129"/>
        <source>Download of dxf libraries failed.
Please install the dxf Library addon manually
from menu Tools -&gt; Addon Manager</source>
        <translation>下載 dxf 資料庫失敗.
請手動安裝 dxf 資料庫附加元件
從功能表 工具 -&gt; 附加元件管理器</translation>
    </message>
    <message>
        <location filename="../../InitGui.py" line="133"/>
        <location filename="../../InitGui.py" line="134"/>
        <location filename="../../InitGui.py" line="135"/>
        <location filename="../../InitGui.py" line="136"/>
        <location filename="../../InitGui.py" line="137"/>
        <source>Draft</source>
        <translation>製圖</translation>
    </message>
    <message>
        <location filename="../../InitGui.py" line="179"/>
        <location filename="../../InitGui.py" line="180"/>
        <location filename="../../InitGui.py" line="181"/>
        <location filename="../../InitGui.py" line="182"/>
        <source>Import-Export</source>
        <translation>匯入-匯出</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_draft_statusbar.py" line="248"/>
        <source>Toggles Grid On/Off</source>
        <translation>開啟/關閉視覺輔助格點</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_draft_statusbar.py" line="265"/>
        <source>Object snapping</source>
        <translation>貼齊物體</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_draft_statusbar.py" line="297"/>
        <source>Toggles Visual Aid Dimensions On/Off</source>
        <translation>開啟/關閉視覺輔助尺寸</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_draft_statusbar.py" line="316"/>
        <source>Toggles Ortho On/Off</source>
        <translation>開啟/關閉正交輔助</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_draft_statusbar.py" line="334"/>
        <source>Toggles Constrain to Working Plane On/Off</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/bspline.py" line="106"/>
        <source>_BSpline.createGeometry: Closed with same first/last Point. Geometry not updated.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/pointarray.py" line="244"/>
        <location filename="../../draftobjects/pointarray.py" line="306"/>
        <source>Point object doesn&apos;t have a discrete point, it cannot be used for an array.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_lineslope.py" line="88"/>
        <location filename="../../draftguitools/gui_lineslope.py" line="91"/>
        <source>Slope</source>
        <translation>斜率</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_clone.py" line="91"/>
        <source>Clone</source>
        <translation>複製</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_hatch.py" line="49"/>
        <source>You must choose a base object before using this command</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="89"/>
        <source>Delete original objects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="95"/>
        <source>Create chamfer</source>
        <translation>建立倒角</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_setstyle.py" line="274"/>
        <source>Save style</source>
        <translation>儲存樣式</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_setstyle.py" line="275"/>
        <source>Name of this new style:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_setstyle.py" line="282"/>
        <source>Warning</source>
        <translation>警告</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_setstyle.py" line="283"/>
        <source>Name exists. Overwrite?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_setstyle.py" line="318"/>
        <source>Error: json module not found. Unable to save style</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_offset.py" line="329"/>
        <source>Offset direction is not defined. Please move the mouse on either side of the object first to indicate a direction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="144"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="156"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="157"/>
        <source>True</source>
        <translation>真(True)</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="148"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="160"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="159"/>
        <source>False</source>
        <translation>偽(False)</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_scale.py" line="151"/>
        <source>Scale</source>
        <translation>縮放</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_scale.py" line="152"/>
        <source>X factor</source>
        <translation>X 因子</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_scale.py" line="153"/>
        <source>Y factor</source>
        <translation>Y 因子</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_scale.py" line="154"/>
        <source>Z factor</source>
        <translation>Z 因子</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_scale.py" line="155"/>
        <source>Uniform scaling</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_scale.py" line="156"/>
        <source>Working plane orientation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_scale.py" line="157"/>
        <source>Copy</source>
        <translation>複製</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_scale.py" line="158"/>
        <source>Modify subelements</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_scale.py" line="159"/>
        <source>Pick from/to points</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_scale.py" line="160"/>
        <source>Create a clone</source>
        <translation>建立一個複製</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_wpproxy.py" line="102"/>
        <source>Writing camera position</source>
        <translation>Writing camera position</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_wpproxy.py" line="119"/>
        <source>Writing objects shown/hidden state</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>DraftCircularArrayTaskPanel</name>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="26"/>
        <source>Circular array</source>
        <translation>圓形陣列</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="44"/>
        <source>(Placeholder for the icon)</source>
        <translation>(圖示佔位符)</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="53"/>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="63"/>
        <source>Distance from one layer of objects to the next layer of objects.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="56"/>
        <source>Radial distance</source>
        <translation>徑向距離</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="76"/>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="87"/>
        <source>Distance from one element in one ring of the array to the next element in the same ring.
It cannot be zero.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="80"/>
        <source>Tangential distance</source>
        <translation>切向距離</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="101"/>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="112"/>
        <source>Number of circular layers or rings to create, including a copy of the original object.
It must be at least 2.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="105"/>
        <source>Number of circular layers</source>
        <translation>圓形層數</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="126"/>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="136"/>
        <source>The number of symmetry lines in the circular array.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="129"/>
        <source>Symmetry</source>
        <translation>對稱</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="151"/>
        <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="155"/>
        <source>Center of rotation</source>
        <translation>旋轉中心</translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="228"/>
        <source>Reset point</source>
        <translation>重置點</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="240"/>
        <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if &quot;Link array&quot; is off.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="244"/>
        <source>Fuse</source>
        <translation>聯集實體</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="251"/>
        <source>If checked, the resulting object will be a &quot;Link array&quot; instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_CircularArray.ui" line="255"/>
        <source>Link array</source>
        <translation>連結陣列</translation>
    </message>
</context>
<context>
    <name>DraftOrthoArrayTaskPanel</name>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="26"/>
        <source>Orthogonal array</source>
        <translation>正交陣列</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="44"/>
        <source>(Placeholder for the icon)</source>
        <translation>(圖示佔位符)</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="51"/>
        <source>Number of elements in the array in the specified direction, including a copy of the original object.
The number must be at least 1 in each direction.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="55"/>
        <source>Number of elements</source>
        <translation>元素總數</translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="124"/>
        <source>X intervals</source>
        <translation>X 間隙</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="197"/>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="288"/>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="379"/>
        <source>Reset the distances.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="200"/>
        <source>Reset X</source>
        <translation>重設 X</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="210"/>
        <source>Distance between the elements in the Y direction.
Normally, only the Y value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="215"/>
        <source>Y intervals</source>
        <translation>Y 間隙</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="291"/>
        <source>Reset Y</source>
        <translation>重設 Y</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="301"/>
        <source>Distance between the elements in the Z direction.
Normally, only the Z value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="306"/>
        <source>Z intervals</source>
        <translation>Z 間隙</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="382"/>
        <source>Reset Z</source>
        <translation>重設 Z</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="394"/>
        <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if &quot;Link array&quot; is off.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="398"/>
        <source>Fuse</source>
        <translation>聯集實體</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="405"/>
        <source>If checked, the resulting object will be a &quot;Link array&quot; instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_OrthoArray.ui" line="409"/>
        <source>Link array</source>
        <translation>連結陣列</translation>
    </message>
</context>
<context>
    <name>DraftPolarArrayTaskPanel</name>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="26"/>
        <source>Polar array</source>
        <translation>極座標陣列</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="44"/>
        <source>(Placeholder for the icon)</source>
        <translation>(圖示佔位符)</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="53"/>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="65"/>
        <source>Sweeping angle of the polar distribution.
A negative angle produces a polar pattern in the opposite direction.
The maximum absolute value is 360 degrees.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="58"/>
        <source>Polar angle</source>
        <translation>極座標角度</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="86"/>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="97"/>
        <source>Number of elements in the array, including a copy of the original object.
It must be at least 2.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="90"/>
        <source>Number of elements</source>
        <translation>元素總數</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="113"/>
        <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="117"/>
        <source>Center of rotation</source>
        <translation>旋轉中心</translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="190"/>
        <source>Reset point</source>
        <translation>重置點</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="202"/>
        <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if &quot;Link array&quot; is off.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="206"/>
        <source>Fuse</source>
        <translation>聯集實體</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="213"/>
        <source>If checked, the resulting object will be a &quot;Link array&quot; instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_PolarArray.ui" line="217"/>
        <source>Link array</source>
        <translation>連結陣列</translation>
    </message>
</context>
<context>
    <name>DraftShapeStringGui</name>
    <message>
        <location filename="../ui/TaskShapeString.ui" line="26"/>
        <source>ShapeString</source>
        <translation>字串造型產生器</translation>
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
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskShapeString.ui" line="120"/>
        <source>Reset Point</source>
        <translation>重置點</translation>
    </message>
    <message>
        <location filename="../ui/TaskShapeString.ui" line="131"/>
        <source>String</source>
        <translation>字串</translation>
    </message>
    <message>
        <location filename="../ui/TaskShapeString.ui" line="138"/>
        <source>Text to be made into ShapeString</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskShapeString.ui" line="149"/>
        <source>Height</source>
        <translation>高度</translation>
    </message>
    <message>
        <location filename="../ui/TaskShapeString.ui" line="156"/>
        <source>Height of the result</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskShapeString.ui" line="176"/>
        <source>Font file</source>
        <translation>字型檔</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskShapeString.ui" line="183"/>
        <source>Font files (*.ttf *.otf *.pfb *.TTF *.OTF *.PFB)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskShapeString.ui" line="183"/>
        <source>Font files (*.ttf *.otf *.pfb)</source>
        <translation>字型檔(*.ttf *.otf *.pfb)</translation>
    </message>
</context>
<context>
    <name>Draft_AddConstruction</name>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="309"/>
        <source>Add to Construction group</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="312"/>
        <source>Adds the selected objects to the construction group,
and changes their appearance to the construction style.
It creates a construction group if it doesn&apos;t exist.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_AddNamedGroup</name>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="361"/>
        <source>Add a new named group</source>
        <translation>新增命名群組</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="365"/>
        <source>Add a new group with a given name.</source>
        <translation>新增已命名群組群組.</translation>
    </message>
</context>
<context>
    <name>Draft_AddPoint</name>
    <message>
        <location filename="../../draftguitools/gui_line_add_delete.py" line="57"/>
        <source>Add point</source>
        <translation>增加點</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_line_add_delete.py" line="60"/>
        <source>Adds a point to an existing Wire or B-spline.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_AddToGroup</name>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="73"/>
        <source>Move to group...</source>
        <translation>移動到群組...</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="76"/>
        <source>Moves the selected objects to an existing group, or removes them from any group.
Create a group first to use this tool.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_AnnotationStyleEditor</name>
    <message>
        <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="78"/>
        <source>Annotation styles...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="82"/>
        <source>Manage or create annotation styles</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_ApplyStyle</name>
    <message>
        <location filename="../../draftguitools/gui_styles.py" line="47"/>
        <source>Apply current style</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_styles.py" line="50"/>
        <source>Applies the current style defined in the toolbar (line width and colors) to the selected objects and groups.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Arc</name>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="66"/>
        <source>Arc</source>
        <translation>弧</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="69"/>
        <source>Creates a circular arc by a center point and a radius.
CTRL to snap, SHIFT to constrain.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_ArcTools</name>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="606"/>
        <source>Arc tools</source>
        <translation>圓弧工具</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="609"/>
        <source>Create various types of circular arcs.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Arc_3Points</name>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="487"/>
        <source>Arc by 3 points</source>
        <translation>3點建立弧</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="490"/>
        <source>Creates a circular arc by picking 3 points.
CTRL to snap, SHIFT to constrain.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Array</name>
    <message>
        <location filename="../../draftguitools/gui_array_simple.py" line="68"/>
        <source>Array</source>
        <translation>矩陣</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_array_simple.py" line="71"/>
        <source>Creates an array from a selected object.
By default, it is a 2x2 orthogonal array.
Once the array is created its type can be changed
to polar or circular, and its properties can be modified.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_ArrayTools</name>
    <message>
        <location filename="../../draftguitools/gui_arrays.py" line="65"/>
        <source>Array tools</source>
        <translation>陣列工具</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arrays.py" line="68"/>
        <source>Create various types of arrays, including rectangular, polar, circular, path, and point</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_AutoGroup</name>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="208"/>
        <source>Autogroup</source>
        <translation>自動群組</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="211"/>
        <source>Select a group to add all Draft and Arch objects to.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_BSpline</name>
    <message>
        <location filename="../../draftguitools/gui_splines.py" line="60"/>
        <source>B-spline</source>
        <translation>貝茲曲線</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_splines.py" line="63"/>
        <source>Creates a multiple-point B-spline. CTRL to snap, SHIFT to constrain.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_BezCurve</name>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="64"/>
        <source>Bézier curve</source>
        <translation>B-雲形曲線</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="67"/>
        <source>Creates an N-degree Bézier curve. The more points you pick, the higher the degree.
CTRL to snap, SHIFT to constrain.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_BezierTools</name>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="475"/>
        <source>Bézier tools</source>
        <translation>B-雲形線工具</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="475"/>
        <source>BÃ©zier tools</source>
        <translation>B-雲形線工具</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="478"/>
        <source>Create various types of Bézier curves.</source>
        <translation>建立各種類型的B-雲形線曲線.</translation>
    </message>
</context>
<context>
    <name>Draft_Circle</name>
    <message>
        <location filename="../../draftguitools/gui_circles.py" line="80"/>
        <source>Circle</source>
        <translation>圓</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_circles.py" line="84"/>
        <source>Creates a circle (full circular arc).
CTRL to snap, ALT to select tangent objects.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_CircularArray</name>
    <message>
        <location filename="../../draftguitools/gui_circulararray.py" line="65"/>
        <source>Circular array</source>
        <translation>圓形陣列</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_circulararray.py" line="68"/>
        <source>Creates copies of the selected object, and places the copies in a radial pattern
creating various circular layers.

The array can be turned into an orthogonal or a polar array by changing its type.</source>
        <translation>建立所選對象的拷貝，並將拷貝放置在建立各種圓形圖層的放射狀圖案中.
陣列可以通過改變其類型變成正交或極坐標陣列.</translation>
    </message>
</context>
<context>
    <name>Draft_Clone</name>
    <message>
        <location filename="../../draftguitools/gui_clone.py" line="70"/>
        <source>Clone</source>
        <translation>複製</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_clone.py" line="73"/>
        <source>Creates a clone of the selected objects.
The resulting clone can be scaled in each of its three directions.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_CubicBezCurve</name>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="242"/>
        <source>Cubic Bézier curve</source>
        <translation>立體B-雲形曲線</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="245"/>
        <source>Creates a Bézier curve made of 2nd degree (quadratic) and 3rd degree (cubic) segments. Click and drag to define each segment.
After the curve is created you can go back to edit each control point and set the properties of each knot.
CTRL to snap, SHIFT to constrain.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_DelPoint</name>
    <message>
        <location filename="../../draftguitools/gui_line_add_delete.py" line="89"/>
        <source>Remove point</source>
        <translation>移除點</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_line_add_delete.py" line="92"/>
        <source>Removes a point from an existing Wire or B-spline.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Dimension</name>
    <message>
        <location filename="../../draftguitools/gui_dimensions.py" line="84"/>
        <source>Dimension</source>
        <translation>標註</translation>
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
You may also select an &apos;App::MeasureDistance&apos; object before launching this command
to turn it into a &apos;Draft Dimension&apos; object.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Downgrade</name>
    <message>
        <location filename="../../draftguitools/gui_downgrade.py" line="58"/>
        <source>Downgrade</source>
        <translation>降級</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_downgrade.py" line="61"/>
        <source>Downgrades the selected objects into simpler shapes.
The result of the operation depends on the types of objects, which may be able to be downgraded several times in a row.
For example, it explodes the selected polylines into simpler faces, wires, and then edges. It can also subtract faces.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Draft2Sketch</name>
    <message>
        <location filename="../../draftguitools/gui_draft2sketch.py" line="57"/>
        <source>Draft to Sketch</source>
        <translation>轉換底圖為草圖</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_draft2sketch.py" line="60"/>
        <source>Convert bidirectionally between Draft objects and Sketches.
Many Draft objects will be converted into a single non-constrained Sketch.
However, a single sketch with disconnected traces will be converted into several individual Draft objects.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Drawing</name>
    <message>
        <location filename="../../draftguitools/gui_drawing.py" line="71"/>
        <source>Drawing</source>
        <translation>工程圖</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_drawing.py" line="74"/>
        <source>Creates a 2D projection on a Drawing Workbench page from the selected objects.
This command is OBSOLETE since the Drawing Workbench became obsolete in 0.17.
Use TechDraw Workbench instead for generating technical drawings.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Edit</name>
    <message>
        <location filename="../../draftguitools/gui_edit.py" line="275"/>
        <source>Edit</source>
        <translation>編輯</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_edit.py" line="284"/>
        <source>Edits the active object.
Press E or ALT+LeftClick to display context menu
on supported nodes and on supported objects.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Ellipse</name>
    <message>
        <location filename="../../draftguitools/gui_ellipses.py" line="59"/>
        <source>Ellipse</source>
        <translation>橢圓</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_ellipses.py" line="62"/>
        <source>Creates an ellipse. CTRL to snap.</source>
        <translation>建立一個橢圓，使用 CTRL 捕捉。</translation>
    </message>
</context>
<context>
    <name>Draft_Facebinder</name>
    <message>
        <location filename="../../draftguitools/gui_facebinders.py" line="60"/>
        <source>Facebinder</source>
        <translation>面連接器</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_facebinders.py" line="62"/>
        <source>Creates a facebinder object from selected faces.</source>
        <translation>由所選面中建立一個面連接器物體。</translation>
    </message>
</context>
<context>
    <name>Draft_Fillet</name>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="64"/>
        <source>Fillet</source>
        <translation>圓角</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="67"/>
        <source>Creates a fillet between two selected wires or edges.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_FlipDimension</name>
    <message>
        <location filename="../../draftguitools/gui_dimension_ops.py" line="62"/>
        <source>Flip dimension</source>
        <translation>旋轉尺寸</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_dimension_ops.py" line="66"/>
        <source>Flip the normal direction of the selected dimensions (linear, radial, angular).
If other objects are selected they are ignored.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Hatch</name>
    <message>
        <location filename="../../draftguitools/gui_hatch.py" line="38"/>
        <source>Hatch</source>
        <translation>填充</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_hatch.py" line="42"/>
        <source>Creates hatches on the faces of a selected object</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Heal</name>
    <message>
        <location filename="../../draftguitools/gui_heal.py" line="58"/>
        <source>Heal</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_heal.py" line="61"/>
        <source>Heal faulty Draft objects saved with an earlier version of the program.
If an object is selected it will try to heal that object in particular,
otherwise it will try to heal all objects in the active document.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Join</name>
    <message>
        <location filename="../../draftguitools/gui_join.py" line="66"/>
        <source>Join</source>
        <translation>組合</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_join.py" line="69"/>
        <source>Joins the selected lines or polylines into a single object.
The lines must share a common point at the start or at the end for the operation to succeed.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Label</name>
    <message>
        <location filename="../../draftguitools/gui_labels.py" line="64"/>
        <source>Label</source>
        <translation>標籤</translation>
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
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Layer</name>
    <message>
        <location filename="../../draftguitools/gui_layers.py" line="52"/>
        <source>Layer</source>
        <translation>圖層</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_layers.py" line="55"/>
        <source>Adds a layer to the document.
Objects added to this layer can share the same visual properties such as line color, line width, and shape color.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Line</name>
    <message>
        <location filename="../../draftguitools/gui_lines.py" line="63"/>
        <source>Line</source>
        <translation>線</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_lines.py" line="66"/>
        <source>Creates a 2-point line. CTRL to snap, SHIFT to constrain.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_LinkArray</name>
    <message>
        <location filename="../../draftguitools/gui_array_simple.py" line="119"/>
        <source>LinkArray</source>
        <translation>連結陣列</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_array_simple.py" line="122"/>
        <source>Like the Array tool, but creates a &apos;Link array&apos; instead.
A &apos;Link array&apos; is more efficient when handling many copies but the &apos;Fuse&apos; option cannot be used.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Mirror</name>
    <message>
        <location filename="../../draftguitools/gui_mirror.py" line="63"/>
        <source>Mirror</source>
        <translation>鏡像</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_mirror.py" line="66"/>
        <source>Mirrors the selected objects along a line defined by two points.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Move</name>
    <message>
        <location filename="../../draftguitools/gui_move.py" line="63"/>
        <source>Move</source>
        <translation>移動</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_move.py" line="66"/>
        <source>Moves the selected objects from one base point to another point.
If the &quot;copy&quot; option is active, it will create displaced copies.
CTRL to snap, SHIFT to constrain.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Offset</name>
    <message>
        <location filename="../../draftguitools/gui_offset.py" line="63"/>
        <source>Offset</source>
        <translation>偏移複製</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_offset.py" line="66"/>
        <source>Offsets of the selected object.
It can also create an offset copy of the original object.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_OrthoArray</name>
    <message>
        <location filename="../../draftguitools/gui_orthoarray.py" line="65"/>
        <source>Array</source>
        <translation>矩陣</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_orthoarray.py" line="68"/>
        <source>Creates copies of the selected object, and places the copies in an orthogonal pattern,
meaning the copies follow the specified direction in the X, Y, Z axes.

The array can be turned into a polar or a circular array by changing its type.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_PathArray</name>
    <message>
        <location filename="../../draftguitools/gui_patharray.py" line="73"/>
        <source>Path array</source>
        <translation>路徑陣列</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_patharray.py" line="76"/>
        <source>Creates copies of the selected object along a selected path.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_PathLinkArray</name>
    <message>
        <location filename="../../draftguitools/gui_patharray.py" line="163"/>
        <source>Path Link array</source>
        <translation>路徑連結陣列</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_patharray.py" line="166"/>
        <source>Like the PathArray tool, but creates a &apos;Link array&apos; instead.
A &apos;Link array&apos; is more efficient when handling many copies but the &apos;Fuse&apos; option cannot be used.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_PathTwistedArray</name>
    <message>
        <location filename="../../draftguitools/gui_pathtwistedarray.py" line="66"/>
        <source>Path twisted array</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_pathtwistedarray.py" line="69"/>
        <source>Creates copies of the selected object along a selected path, and twists the copies.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_PathTwistedLinkArray</name>
    <message>
        <location filename="../../draftguitools/gui_pathtwistedarray.py" line="121"/>
        <source>Path twisted Link array</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_pathtwistedarray.py" line="124"/>
        <source>Like the PathTwistedArray tool, but creates a &apos;Link array&apos; instead.
A &apos;Link array&apos; is more efficient when handling many copies but the &apos;Fuse&apos; option cannot be used.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Point</name>
    <message>
        <location filename="../../draftguitools/gui_points.py" line="62"/>
        <source>Point</source>
        <translation>點</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_points.py" line="65"/>
        <source>Creates a point object. Click anywhere on the 3D view.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_PointArray</name>
    <message>
        <location filename="../../draftguitools/gui_pointarray.py" line="76"/>
        <source>Point array</source>
        <translation>點陣列</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_pointarray.py" line="79"/>
        <source>Creates copies of the selected object, and places the copies at the position of various points.

The points need to be grouped under a compound of points before using this tool.
To create this compound, select various points and then use the Part Compound tool,
or use the Draft Upgrade tool to create a &apos;Block&apos;, or create a Sketch and add simple points to it.

Select the base object, and then select the compound or the sketch to create the point array.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_PointLinkArray</name>
    <message>
        <location filename="../../draftguitools/gui_pointarray.py" line="141"/>
        <source>PointLinkArray</source>
        <translation>點連結陣列</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_pointarray.py" line="144"/>
        <source>Like the PointArray tool, but creates a &apos;Point link array&apos; instead.
A &apos;Point link array&apos; is more efficient when handling many copies.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_PolarArray</name>
    <message>
        <location filename="../../draftguitools/gui_polararray.py" line="65"/>
        <source>Polar array</source>
        <translation>極座標陣列</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_polararray.py" line="68"/>
        <source>Creates copies of the selected object, and places the copies in a polar pattern
defined by a center of rotation and its angle.

The array can be turned into an orthogonal or a circular array by changing its type.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Polygon</name>
    <message>
        <location filename="../../draftguitools/gui_polygons.py" line="57"/>
        <source>Polygon</source>
        <translation>多邊形</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_polygons.py" line="60"/>
        <source>Creates a regular polygon (triangle, square, pentagon, ...), by defining the number of sides and the circumscribed radius.
CTRL to snap, SHIFT to constrain</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Rectangle</name>
    <message>
        <location filename="../../draftguitools/gui_rectangles.py" line="54"/>
        <source>Rectangle</source>
        <translation>矩形</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_rectangles.py" line="57"/>
        <source>Creates a 2-point rectangle. CTRL to snap.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Rotate</name>
    <message>
        <location filename="../../draftguitools/gui_rotate.py" line="62"/>
        <source>Rotate</source>
        <translation>旋轉</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_rotate.py" line="65"/>
        <source>Rotates the selected objects. Choose the center of rotation, then the initial angle, and then the final angle.
If the &quot;copy&quot; option is active, it will create rotated copies.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Scale</name>
    <message>
        <location filename="../../draftguitools/gui_scale.py" line="71"/>
        <source>Scale</source>
        <translation>縮放</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_scale.py" line="74"/>
        <source>Scales the selected objects from a base point.
CTRL to snap, SHIFT to constrain, ALT to copy.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_SelectGroup</name>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="164"/>
        <source>Select group</source>
        <translation>選擇群組</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="167"/>
        <source>Selects the contents of selected groups. For selected non-group objects, the contents of the group they are in is selected.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_SelectPlane</name>
    <message>
        <location filename="../../draftguitools/gui_selectplane.py" line="65"/>
        <source>Select Plane</source>
        <translation>選取平面</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_selectplane.py" line="67"/>
        <source>Select the face of solid body to create a working plane on which to sketch Draft objects.
You may also select a three vertices or a Working Plane Proxy.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_SetStyle</name>
    <message>
        <location filename="../../draftguitools/gui_setstyle.py" line="55"/>
        <source>Set style</source>
        <translation>設定樣式</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_setstyle.py" line="57"/>
        <source>Sets default styles</source>
        <translation>設定預設樣式</translation>
    </message>
</context>
<context>
    <name>Draft_Shape2DView</name>
    <message>
        <location filename="../../draftguitools/gui_shape2dview.py" line="59"/>
        <source>Shape 2D view</source>
        <translation>2D造型視景</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_shape2dview.py" line="62"/>
        <source>Creates a 2D projection of the selected objects on the XY plane.
The initial projection direction is the negative of the current active view direction.
You can select individual faces to project, or the entire solid, and also include hidden lines.
These projections can be used to create technical drawings with the TechDraw Workbench.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_ShapeString</name>
    <message>
        <location filename="../../draftguitools/gui_shapestrings.py" line="66"/>
        <source>Shape from text</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_shapestrings.py" line="68"/>
        <source>Creates a shape from a text string by choosing a specific font and a placement.
The closed shapes can be used for extrusions and boolean operations.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_ShowSnapBar</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="589"/>
        <source>Show snap toolbar</source>
        <translation>顯示貼齊工具列</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="592"/>
        <source>Show the snap toolbar if it is hidden.</source>
        <translation>隱藏時，將顯示貼齊工具列。</translation>
    </message>
</context>
<context>
    <name>Draft_Slope</name>
    <message>
        <location filename="../../draftguitools/gui_lineslope.py" line="70"/>
        <source>Set slope</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_lineslope.py" line="73"/>
        <source>Sets the slope of the selected line by changing the value of the Z value of one of its points.
If a polyline is selected, it will apply the slope transformation to each of its segments.

The slope will always change the Z value, therefore this command only works well for
straight Draft lines that are drawn in the XY plane. Selected objects that aren&apos;t single lines will be ignored.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Angle</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="344"/>
        <source>Angle</source>
        <translation>角度</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="347"/>
        <source>Set snapping to points in a circular arc located at multiples of 30 and 45 degree angles.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Center</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="374"/>
        <source>Center</source>
        <translation>中心</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="377"/>
        <source>Set snapping to the center of a circular arc.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Dimensions</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="526"/>
        <source>Show dimensions</source>
        <translation>顯示尺寸</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="529"/>
        <source>Show temporary linear dimensions when editing an object and using other snapping methods.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Endpoint</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="313"/>
        <source>Endpoint</source>
        <translation>端點</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="316"/>
        <source>Set snapping to endpoints of an edge.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Extension</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="404"/>
        <source>Extension</source>
        <translation>延伸</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="407"/>
        <source>Set snapping to the extension of an edge.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Grid</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="223"/>
        <source>Grid</source>
        <translation>格線</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="226"/>
        <source>Set snapping to the intersection of grid lines.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Intersection</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="253"/>
        <source>Intersection</source>
        <translation>交集</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="256"/>
        <source>Set snapping to the intersection of edges.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Lock</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="133"/>
        <source>Main snapping toggle On/Off</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="136"/>
        <source>Activates or deactivates all snap methods at once.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Midpoint</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="163"/>
        <source>Midpoint</source>
        <translation>中點</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="166"/>
        <source>Set snapping to the midpoint of an edge.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Near</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="434"/>
        <source>Nearest</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="437"/>
        <source>Set snapping to the nearest point of an edge.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Ortho</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="465"/>
        <source>Orthogonal</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="468"/>
        <source>Set snapping to a direction that is a multiple of 45 degrees from a point.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Parallel</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="283"/>
        <source>Parallel</source>
        <translation>平行</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="286"/>
        <source>Set snapping to a direction that is parallel to an edge.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Perpendicular</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="193"/>
        <source>Perpendicular</source>
        <translation>垂直</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="196"/>
        <source>Set snapping to a direction that is perpendicular to an edge.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_Special</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="495"/>
        <source>Special</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="498"/>
        <source>Set snapping to the special points defined inside an object.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Snap_WorkingPlane</name>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="559"/>
        <source>Working plane</source>
        <translation>工作平面</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="562"/>
        <source>Restricts snapping to a point in the current working plane.
If you select a point outside the working plane, for example, by using other snapping methods,
it will snap to that point&apos;s projection in the current working plane.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Split</name>
    <message>
        <location filename="../../draftguitools/gui_split.py" line="56"/>
        <source>Split</source>
        <translation>拆分</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_split.py" line="59"/>
        <source>Splits the selected line or polyline into two independent lines
or polylines by clicking anywhere along the original object.
It works best when choosing a point on a straight segment and not a corner vertex.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Stretch</name>
    <message>
        <location filename="../../draftguitools/gui_stretch.py" line="64"/>
        <source>Stretch</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_stretch.py" line="67"/>
        <source>Stretches the selected objects.
Select an object, then draw a rectangle to pick the vertices that will be stretched,
then draw a line to specify the distance and direction of stretching.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_SubelementHighlight</name>
    <message>
        <location filename="../../draftguitools/gui_subelements.py" line="61"/>
        <source>Subelement highlight</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_subelements.py" line="64"/>
        <source>Highlight the subelements of the selected objects, so that they can then be edited with the move, rotate, and scale tools.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Text</name>
    <message>
        <location filename="../../draftguitools/gui_texts.py" line="60"/>
        <source>Text</source>
        <translation>文字</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_texts.py" line="63"/>
        <source>Creates a multi-line annotation. CTRL to snap.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_ToggleConstructionMode</name>
    <message>
        <location filename="../../draftguitools/gui_togglemodes.py" line="99"/>
        <source>Toggle construction mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_togglemodes.py" line="102"/>
        <source>Toggles the Construction mode.
When this is active, the following objects created will be included in the construction group, and will be drawn with the specified color and properties.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_ToggleContinueMode</name>
    <message>
        <location filename="../../draftguitools/gui_togglemodes.py" line="131"/>
        <source>Toggle continue mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_togglemodes.py" line="133"/>
        <source>Toggles the Continue mode.
When this is active, any drawing tool that is terminated will automatically start again.
This can be used to draw several objects one after the other in succession.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_ToggleDisplayMode</name>
    <message>
        <location filename="../../draftguitools/gui_togglemodes.py" line="166"/>
        <source>Toggle normal/wireframe display</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_togglemodes.py" line="168"/>
        <source>Switches the display mode of selected objects from flatlines to wireframe and back.
This is helpful to quickly visualize objects that are hidden by other objects.
This is intended to be used with closed shapes and solids, and doesn&apos;t affect open wires.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_ToggleGrid</name>
    <message>
        <location filename="../../draftguitools/gui_grid.py" line="58"/>
        <source>Toggle grid</source>
        <translation>切換格線</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_grid.py" line="59"/>
        <source>Toggles the Draft grid on and off.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Trimex</name>
    <message>
        <location filename="../../draftguitools/gui_trimex.py" line="76"/>
        <source>Trimex</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_trimex.py" line="82"/>
        <source>Trims or extends the selected object, or extrudes single faces.
CTRL snaps, SHIFT constrains to current segment or to normal, ALT inverts.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Upgrade</name>
    <message>
        <location filename="../../draftguitools/gui_upgrade.py" line="58"/>
        <source>Upgrade</source>
        <translation>升級</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_upgrade.py" line="61"/>
        <source>Upgrades the selected objects into more complex shapes.
The result of the operation depends on the types of objects, which may be able to be upgraded several times in a row.
For example, it can join the selected objects into one, convert simple edges into parametric polylines,
convert closed edges into filled faces and parametric polygons, and merge faces into a single face.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_Wire</name>
    <message>
        <location filename="../../draftguitools/gui_lines.py" line="306"/>
        <source>Polyline</source>
        <translation>聚合線</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_lines.py" line="309"/>
        <source>Creates a multiple-points line (polyline). CTRL to snap, SHIFT to constrain.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_WireToBSpline</name>
    <message>
        <location filename="../../draftguitools/gui_wire2spline.py" line="64"/>
        <source>Wire to B-spline</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_wire2spline.py" line="67"/>
        <source>Converts a selected polyline to a B-spline, or a B-spline to a polyline.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Draft_WorkingPlaneProxy</name>
    <message>
        <location filename="../../draftguitools/gui_planeproxy.py" line="50"/>
        <source>Create working plane proxy</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_planeproxy.py" line="52"/>
        <source>Creates a proxy object from the current working plane.
Once the object is created double click it in the tree view to restore the camera position and objects&apos; visibilities.
Then you can use it to save a different camera position and objects&apos; states any time you need.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Form</name>
    <message>
        <location filename="../../Resources/ui/dialogHatch.ui" line="14"/>
        <source>Hatch</source>
        <translation>填充</translation>
    </message>
    <message>
        <location filename="../ui/dialogHatch.ui" line="20"/>
        <source>PAT file:</source>
        <translation>PAT 檔案:</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/dialogHatch.ui" line="27"/>
        <source>Pattern files (*.pat *.PAT)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/dialogHatch.ui" line="14"/>
        <source>Form</source>
        <translation>格式</translation>
    </message>
    <message>
        <location filename="../ui/dialogHatch.ui" line="27"/>
        <source>pattern files (*.pat)</source>
        <translation>圖樣檔 (*.pat)</translation>
    </message>
    <message>
        <location filename="../ui/dialogHatch.ui" line="34"/>
        <source>Pattern:</source>
        <translation>圖樣:</translation>
    </message>
    <message>
        <location filename="../ui/dialogHatch.ui" line="44"/>
        <source>Scale</source>
        <translation>縮放</translation>
    </message>
    <message>
        <location filename="../ui/dialogHatch.ui" line="71"/>
        <source>°</source>
        <translation>°</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/dialogHatch.ui" line="44"/>
        <source>Scale:</source>
        <translation>縮放:</translation>
    </message>
    <message>
        <location filename="../ui/dialogHatch.ui" line="64"/>
        <source>Rotation:</source>
        <translation>旋轉：</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="14"/>
        <source>Style settings</source>
        <translation>樣式設定</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="22"/>
        <source>Fills the values below with a stored style preset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="26"/>
        <source>Load preset</source>
        <translation>載入預設</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="40"/>
        <source>Save current style as a preset...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="57"/>
        <source>Lines and faces</source>
        <translation>線和面</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="66"/>
        <source>Line color</source>
        <translation>線條顏色</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="73"/>
        <source>The color of lines</source>
        <translation>線條顏色</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="80"/>
        <source>Line width</source>
        <translation>線寬</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="87"/>
        <source> px</source>
        <translation> px</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="94"/>
        <source>Draw style</source>
        <translation>繪製樣式</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="101"/>
        <source>The line style</source>
        <translation>線條樣式</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="105"/>
        <source>Solid</source>
        <translation>實體</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="110"/>
        <source>Dashed</source>
        <translation>虛線</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="115"/>
        <source>Dotted</source>
        <translation>點</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="120"/>
        <source>DashDot</source>
        <translation>虛線點</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="128"/>
        <source>Display mode</source>
        <translation>顯示模式</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="135"/>
        <source>The display mode for faces</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="139"/>
        <source>Flat Lines</source>
        <translation>框線</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="144"/>
        <source>Wireframe</source>
        <translation>線框</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="149"/>
        <source>Shaded</source>
        <translation>上色</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="186"/>
        <source> %</source>
        <translation> %</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="397"/>
        <source>Texts/dims</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="22"/>
        <source>Fill the values below from a stored style preset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="40"/>
        <source>Save the current style as a preset...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="56"/>
        <source>Shapes</source>
        <translation>造型</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="118"/>
        <source>Point color</source>
        <translation>點顏色</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="129"/>
        <source>Point size</source>
        <translation>點大小</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="154"/>
        <source>Points</source>
        <translation>點</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="221"/>
        <source>The color for texts, dimension texts and label texts</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="228"/>
        <source>Font name</source>
        <translation>字體名稱</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="241"/>
        <source>The font for texts, dimensions and labels</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="248"/>
        <source>Font size</source>
        <translation>字體大小</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="255"/>
        <source>The height for texts, dimension texts and label texts</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="272"/>
        <source>The line spacing for multi-line texts and labels (relative to the font size)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="279"/>
        <source>Scale multiplier</source>
        <translation>規模乘數</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="286"/>
        <source>The annotation scale multiplier is the inverse of the scale set in the 
Annotation scale widget. If the scale is 1:100 the multiplier is 100.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="303"/>
        <source>Line and arrow color</source>
        <translation>線條和箭頭顏色</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="328"/>
        <source>Arrow type</source>
        <translation>箭頭樣式</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="433"/>
        <source>The length of extension lines. Use 0 for full extension lines. A negative value
defines the gap between the ends of the extension lines and the measured points.
A positive value defines the maximum length of the extension lines. Only used
for linear dimensions.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="453"/>
        <source>The length of extension lines above the dimension line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="470"/>
        <source>The space between the dimension line and the dimension text</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="485"/>
        <source>Apply the above style to selected object(s)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="499"/>
        <source>Apply the above style to all annotations (texts, dimensions and labels)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="162"/>
        <source>Shape color</source>
        <translation>形狀色彩</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="169"/>
        <source>The color of faces</source>
        <translation>面的顏色</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="176"/>
        <source>Transparency</source>
        <translation>透明度</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="183"/>
        <source>The transparency of faces</source>
        <translation>面透明度</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="199"/>
        <source>Annotations</source>
        <translation>註記</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="232"/>
        <source>The size of texts and dimension texts</source>
        <translation>文字與尺寸文字的大小</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="266"/>
        <source>The color of texts and dimension texts</source>
        <translation>文字與尺寸文字的顏色</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="205"/>
        <source>Text font</source>
        <translation>文字字體</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="225"/>
        <source>Text size</source>
        <translation>字體大小</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="273"/>
        <source>Line spacing</source>
        <translation>行距</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="259"/>
        <source>Text color</source>
        <translation>文字顏色</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="218"/>
        <source>The font to use for texts and dimensions</source>
        <translation>文字與尺寸使用的字體</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="280"/>
        <source>The spacing between different lines of text</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskPanel_SetStyle.ui" line="272"/>
        <source>Dimensions</source>
        <translation>尺寸標註</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="287"/>
        <source>Arrow style</source>
        <translation>箭頭樣式</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="249"/>
        <source>The space between the text and the dimension line</source>
        <translation>文字與尺寸線的間距</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="294"/>
        <source>The type of dimension arrows</source>
        <translation>尺寸箭頭的樣式</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="298"/>
        <source>Dot</source>
        <translation>點</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="303"/>
        <source>Circle</source>
        <translation>圓</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="308"/>
        <source>Arrow</source>
        <translation>箭頭</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="313"/>
        <source>Tick</source>
        <translation>鉤號</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="318"/>
        <source>Tick-2</source>
        <translation>鉤號-2</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="385"/>
        <source>If checked, a unit symbol is added to dimension texts</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/TaskPanel_SetStyle.ui" line="402"/>
        <source>The unit override for dimensions. Leave blank to use the current FreeCAD unit.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskPanel_SetStyle.ui" line="327"/>
        <source>Ext lines</source>
        <translation>延伸線</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="333"/>
        <source>The size of dimension arrows</source>
        <translation>尺寸箭頭的大小</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="350"/>
        <source>If the unit suffix is shown on dimension texts or not</source>
        <translation>單位後綴是否顯示於尺寸文字上</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="242"/>
        <source>Text spacing</source>
        <translation>文字間距</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskPanel_SetStyle.ui" line="364"/>
        <source>The distance the dimension line is extended past the extension lines</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="326"/>
        <source>Arrow size</source>
        <translation>箭頭尺寸</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskPanel_SetStyle.ui" line="381"/>
        <source>Dim overshoot</source>
        <translation>尺寸超越量</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskPanel_SetStyle.ui" line="388"/>
        <source>Length of the extension lines beyond the dimension line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskPanel_SetStyle.ui" line="398"/>
        <source>Length of the extension lines</source>
        <translation>延伸線長度</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="343"/>
        <source>Show unit</source>
        <translation>顯示單位</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskPanel_SetStyle.ui" line="415"/>
        <source>Ext overshoot</source>
        <translation>延伸超越量</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="363"/>
        <source>Unit override</source>
        <translation>單位覆蓋</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="370"/>
        <source>The unit to use for dimensions. Leave blank to use current FreeCAD unit</source>
        <translation>尺寸標示使用的單位。 留空以使用目前 FreeCAD 的單位</translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="382"/>
        <source>Apply above style to selected object(s)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskPanel_SetStyle.ui" line="385"/>
        <source>Selected</source>
        <translation>已選取</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskPanel_SetStyle.ui" line="455"/>
        <source>Apply above style to all annotations (texts, dimensions and labels)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="14"/>
        <source>Working plane setup</source>
        <translation>工作平面設定</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskSelectPlane.ui" line="20"/>
        <source>Select 3 vertices, one or more shapes or a WP Proxy. Then confirm by clicking in the 3D view.
Or choose one of the options below.</source>
        <translation>選擇 3 個頂點、一個或多個形狀或 WP 代理程式. 然後透過點擊 3D 檢視進行確認.
或選擇以下選項之一.</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="20"/>
        <source>Select a face or working plane proxy or 3 vertices.
Or choose one of the options below</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="31"/>
        <source>Sets the working plane to the XY plane (ground plane)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="34"/>
        <source>Top (XY)</source>
        <translation>XY （上）</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="41"/>
        <source>Sets the working plane to the XZ plane (front plane)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="44"/>
        <source>Front (XZ)</source>
        <translation>XZ (正面)</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="51"/>
        <source>Sets the working plane to the YZ plane (side plane)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="54"/>
        <source>Side (YZ)</source>
        <translation>YZ（側面）</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="61"/>
        <source>Sets the working plane facing the current view</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="64"/>
        <source>Align to view</source>
        <translation>對齊檢視</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="71"/>
        <source>The working plane will align to the current
view each time a command is started</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="78"/>
        <source>Automatic</source>
        <translation>自動</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="87"/>
        <source>Offset</source>
        <translation>偏移複製</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="94"/>
        <source>An optional offset to give to the working plane
above its base position. Use this together with one
of the buttons above</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="106"/>
        <location filename="../ui/TaskSelectPlane.ui" line="118"/>
        <source>If this is selected, the working plane will be
centered on the current view when pressing one
of the buttons above</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="111"/>
        <source>Center plane on view</source>
        <translation>視野中心平面</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskSelectPlane.ui" line="135"/>
        <source>Or select a single vertex to move the current working plane without changing its orientation. Then press the button below.</source>
        <translation>或選擇單一頂點來移動目前工作平面而不改變其方向. 然後按下下面的按鈕.</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskSelectPlane.ui" line="264"/>
        <source>Resets the working plane to its next position</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/TaskSelectPlane.ui" line="267"/>
        <source>Next</source>
        <translation>下一個</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="135"/>
        <source>Or select a single vertex to move the current
working plane without changing its orientation.
Then, press the button below</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="147"/>
        <source>Moves the working plane without changing its
orientation. If no point is selected, the plane
will be moved to the center of the view</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="152"/>
        <source>Move working plane</source>
        <translation>移動工作平面</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="161"/>
        <location filename="../ui/TaskSelectPlane.ui" line="171"/>
        <source>The spacing between the smaller grid lines</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="164"/>
        <source>Grid spacing</source>
        <translation>網格間距</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="181"/>
        <location filename="../ui/TaskSelectPlane.ui" line="191"/>
        <source>The number of squares between each main line of the grid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="184"/>
        <source>Main line every</source>
        <translation>主線每個</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="198"/>
        <source>Grid extension</source>
        <translation>網格延伸</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="205"/>
        <source> lines</source>
        <translation> 線段</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="218"/>
        <location filename="../ui/TaskSelectPlane.ui" line="230"/>
        <source>The distance at which a point can be snapped to
when approaching the mouse. You can also change this
value by using the [ and ] keys while drawing</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="223"/>
        <source>Snapping radius</source>
        <translation>捕捉半徑</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="241"/>
        <source>Centers the view on the current working plane</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="244"/>
        <source>Center view</source>
        <translation>中間視角</translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="251"/>
        <source>Resets the working plane to its previous position</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/TaskSelectPlane.ui" line="254"/>
        <source>Previous</source>
        <translation>前一個</translation>
    </message>
</context>
<context>
    <name>Gui::Dialog::DlgSettingsDraft</name>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="14"/>
        <source>User interface settings</source>
        <translation>使用者介面設定</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="26"/>
        <source>In-Command Shortcuts</source>
        <translation>命令內快捷鍵</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="14"/>
        <source>Interface</source>
        <translation>介面</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="20"/>
        <source>In-command shortcuts</source>
        <translation>命令內快捷鍵</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="37"/>
        <source>Relative</source>
        <translation>相對關係</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="59"/>
        <source>R</source>
        <translation>R</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="183"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="202"/>
        <source>Select edge</source>
        <translation>選擇邊緣</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="234"/>
        <source>Subelement mode</source>
        <translation>子元素模式</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="247"/>
        <source>B</source>
        <translation>B</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="279"/>
        <source>C</source>
        <translation>C</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="81"/>
        <source>Continue</source>
        <translation>繼續</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="439"/>
        <source>N</source>
        <translation>N</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="458"/>
        <source>Cycle snap</source>
        <translation>循環捕捉</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="490"/>
        <source>Add hold</source>
        <translation>新增保留</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="522"/>
        <source>Set working plane</source>
        <translation>設定工作平面</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="586"/>
        <source>Increase radius</source>
        <translation>增加半徑</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="618"/>
        <source>Decrease radius</source>
        <translation>減少半徑</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="749"/>
        <source>UI options</source>
        <translation>使用者介面選項</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="755"/>
        <source>If checked, the Draft snap toolbar will only be visible during commands</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="758"/>
        <source>Only show the Draft snap toolbar during commands</source>
        <translation>僅在指令期間顯示草稿捕捉工具列</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="774"/>
        <source>If checked, the Snap widget is displayed in the Draft statusbar</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="777"/>
        <source>Show the Snap widget in the Draft Workbench</source>
        <translation>在草稿工作台中顯示捕捉小工具</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="793"/>
        <source>If checked, the Annotation scale widget is displayed in the Draft statusbar</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftinterface.ui" line="796"/>
        <source>Show the Annotation scale widget in the Draft Workbench</source>
        <translation>在草稿工作台中顯示註釋比例小工具</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="103"/>
        <source>T</source>
        <translation>T</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="125"/>
        <source>Close</source>
        <translation>關閉</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="147"/>
        <source>O</source>
        <translation>O</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="169"/>
        <source>Copy</source>
        <translation>複製</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="191"/>
        <source>P</source>
        <translation>P</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="213"/>
        <source>Subelement Mode</source>
        <translation>子元素模式</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="235"/>
        <source>D</source>
        <translation>D</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="257"/>
        <source>Fill</source>
        <translation>填充</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="279"/>
        <source>L</source>
        <translation>L</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="301"/>
        <source>Exit</source>
        <translation>結束離開</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="323"/>
        <source>A</source>
        <translation>A</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="345"/>
        <source>Select Edge</source>
        <translation>選擇邊緣</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="367"/>
        <source>E</source>
        <translation>E</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="389"/>
        <source>Add Hold</source>
        <translation>新增保留</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="411"/>
        <source>Q</source>
        <translation>Q</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="433"/>
        <source>Length</source>
        <translation>間距</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="455"/>
        <source>H</source>
        <translation>H</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="477"/>
        <source>Wipe</source>
        <translation>擦拭</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="499"/>
        <source>W</source>
        <translation>W</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="521"/>
        <source>Set WP</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="543"/>
        <source>U</source>
        <translation>U</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="565"/>
        <source>Cycle Snap</source>
        <translation>循環捕捉</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="587"/>
        <source>`</source>
        <translation>`</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="609"/>
        <source>Global</source>
        <translation>全局的</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="631"/>
        <source>G</source>
        <translation>G</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="719"/>
        <source>[</source>
        <translation>[</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="763"/>
        <source>]</source>
        <translation>]</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="928"/>
        <source>Enable draft statusbar customization</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-draftinterface.ui" line="674"/>
        <source>Undo</source>
        <translation>復原</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="653"/>
        <source>Snap</source>
        <translation>貼齊</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="675"/>
        <source>S</source>
        <translation>S</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="697"/>
        <source>Increase Radius</source>
        <translation>增加半徑</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="741"/>
        <source>Decrease Radius</source>
        <translation>減少半徑</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="785"/>
        <source>Restrict X</source>
        <translation>限定 X</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="807"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="829"/>
        <source>Restrict Y</source>
        <translation>限定 Y</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="851"/>
        <source>Y</source>
        <translation>Y</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="873"/>
        <source>Restrict Z</source>
        <translation>限定 Z</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="895"/>
        <source>Z</source>
        <translation>Z</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="931"/>
        <source>Draft Statusbar</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="951"/>
        <source>Enable snap statusbar widget</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="954"/>
        <source>Draft snap widget</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="970"/>
        <source>Enable draft statusbar annotation scale widget</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftinterface.ui" line="973"/>
        <source>Annotation scale widget</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="14"/>
        <source>Grid and snapping</source>
        <translation>格線與鎖點</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="26"/>
        <source>If checked, the grid will always be visible in new views.
Use Draft ToggleGrid to change this for the active view.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="49"/>
        <source>If checked, the grid will be visible during commands in new views.
Use Draft ToggleGrid to change this for the active view.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="53"/>
        <source>Show the grid during commands</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="69"/>
        <source>If checked, an additional border is displayed around the grid,
showing the main square size in the bottom left corner</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="92"/>
        <source>If checked, the outline of a human figure is displayed at the bottom left
corner of the grid. Only effective if the BIM workbench is installed and
&quot;Show grid border&quot; is enabled.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="113"/>
        <source>If checked, the two main axes of the grid are colored red, green or blue
if they match the X, Y or Z axis of the global coordinate system</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="146"/>
        <source>The number of squares between main grid lines.
These lines are thicker than normal grid lines.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="177"/>
        <source>The distance between grid lines</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="209"/>
        <source>The number of horizontal and vertical lines in the grid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="231"/>
        <source>Grid transparency</source>
        <translation>網格透明度</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="241"/>
        <source> %</source>
        <translation> %</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="257"/>
        <source>Grid color</source>
        <translation>網格顏色</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="287"/>
        <source>Snapping and modifier keys</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="293"/>
        <source>Snap symbol style</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="300"/>
        <source>The style for snap symbols</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="330"/>
        <source>Snap symbol color</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="337"/>
        <source>The color for snap symbols</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="357"/>
        <source>If checked, snapping is activated without the need to press the Snap modifier key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="360"/>
        <source>Always snap</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="379"/>
        <source>Snap modifier</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="395"/>
        <source>The Snap modifier key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="426"/>
        <source>Constrain modifier</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="433"/>
        <source>The Constrain modifier key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="461"/>
        <source>Alt modifier</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftsnap.ui" line="468"/>
        <source>The Alt modifier key. The function of this key depends on the command.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="35"/>
        <source>Snapping</source>
        <translation>對齊</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="43"/>
        <source>If this is checked, snapping is activated without the need to press the snap mod key</source>
        <translation>若勾選此項，無需使用鎖點模式鍵即可鎖點</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="46"/>
        <source>Always snap (disable snap mod)</source>
        <translation>持續鎖點(取消鎖點模式)</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="66"/>
        <source>Constrain mod</source>
        <translation>拘束模式</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="86"/>
        <source>The Constraining modifier key</source>
        <translation>拘束編輯鍵</translation>
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
        <translation>鎖點模式</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="138"/>
        <source>The snap modifier key</source>
        <translation>鎖點編輯鍵</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="173"/>
        <source>Alt mod</source>
        <translation>Alt模式</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="193"/>
        <source>The Alt modifier key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="228"/>
        <source>If checked, the Snap toolbar will be shown whenever you use snapping</source>
        <translation>若勾選,當您使用快選時會顯示快選工具列</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="231"/>
        <source>Show Draft Snap toolbar</source>
        <translation>顯示底圖快選工具列</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="251"/>
        <source>Hide Draft snap toolbar after use</source>
        <translation>於使用後隱藏底圖鎖點工具列</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="272"/>
        <source>Grid</source>
        <translation>格線</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="278"/>
        <source>If checked, a grid will appear when drawing</source>
        <translation>若勾選,於繪圖時會顯示網格</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="281"/>
        <source>Use grid</source>
        <translation>使用網格</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="300"/>
        <source>If checked, the Draft grid will always be visible when the Draft workbench is active. Otherwise only when using a command</source>
        <translation>若勾選,底圖格線當底圖工作區啟動時皆會顯示,否則僅要求時顯示</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="303"/>
        <source>Always show the grid</source>
        <translation>永遠顯示格線</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="319"/>
        <source>If checked, an additional border is displayed around the grid, showing the main square size in the bottom left border</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="322"/>
        <source>Show grid border</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="338"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;If checked, the outline of a human figure is displayed at the bottom left corner of the grid. This option is only effective if the BIM workbench is installed and if &amp;quot;Show grid border&amp;quot; option is enabled.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="341"/>
        <source>Show human figure</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="357"/>
        <source>If set, the grid will have its two main axes colored in red, green or blue when they match global axes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="360"/>
        <source>Use colored axes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="381"/>
        <source>Main lines every</source>
        <translation>所有主要線段</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="404"/>
        <source>Mainlines will be drawn thicker. Specify here how many squares between mainlines.</source>
        <translation>主線較粗,特別於主線間有許多方形時.</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="430"/>
        <source>Grid spacing</source>
        <translation>網格間距</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="453"/>
        <source>The spacing between each grid line</source>
        <translation>格線間距</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="485"/>
        <source>Grid size</source>
        <translation>網格尺寸</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="505"/>
        <source>The number of horizontal or vertical lines of the grid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="511"/>
        <source> lines</source>
        <translation> 線段</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="534"/>
        <source>Grid color and transparency</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="554"/>
        <source>The color of the grid</source>
        <translation>網格顏色</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="574"/>
        <source>The overall transparency of the grid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="595"/>
        <source>Draft Edit preferences</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="598"/>
        <source>Edit</source>
        <translation>編輯</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="621"/>
        <source>Maximum number of contemporary edited objects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="644"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Sets the maximum number of objects Draft Edit&lt;/p&gt;&lt;p&gt;can process at the same time&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="691"/>
        <source>Draft edit pick radius</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftsnap.ui" line="714"/>
        <source>Controls pick radius of edit nodes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="14"/>
        <source>Texts and dimensions</source>
        <translation>文字和尺寸</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="26"/>
        <source>Text settings</source>
        <translation>文字設定</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="34"/>
        <source>Font family</source>
        <translation>字型家族</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="47"/>
        <source>This is the default font name for all Draft texts and dimensions.
It can be a font name such as &quot;Arial&quot;, a default style such as &quot;sans&quot;, &quot;serif&quot;
or &quot;mono&quot;, or a family such as &quot;Arial,Helvetica,sans&quot; or a name with a style
such as &quot;Arial:Bold&quot;</source>
        <translation>這是所有底圖文字和標註的預設字體名稱。如&quot;Arial&quot;、預設樣式如&quot;san&quot;、&quot;serif&quot;或&quot;mono&quot;或一個家族&quot;&quot;Arial,Helvetica,sans&quot;或名稱的樣式如&quot;arial：粗體&quot;</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="144"/>
        <source>text above (2D)</source>
        <translation>文字在上(2D)</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="149"/>
        <source> text inside (3D)</source>
        <translation> 文字在內(3D)</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="20"/>
        <source>Texts</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="26"/>
        <source>Font name or family</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="33"/>
        <source>The default font for texts, dimensions and labels. It can be a font name such
as &quot;Arial&quot;, a style such as &quot;sans&quot;, &quot;serif&quot; or &quot;mono&quot;, or a family such as
&quot;Arial,Helvetica,sans&quot;, or a name with a style such as &quot;Arial:Bold&quot;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="59"/>
        <source>Internal font</source>
        <translation>內部字型</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="76"/>
        <source>Font size</source>
        <translation>字體大小</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="67"/>
        <source>The default height for texts, dimension texts and label texts</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="99"/>
        <source>Line spacing factor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="106"/>
        <source>The default line spacing for multi-line texts and labels (relative to the font size)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="125"/>
        <source>Scale multiplier</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="132"/>
        <source>The default annotation scale multiplier. This is the inverse of the scale set
in the Annotation scale widget. If the scale is 1:100 the multiplier is 100.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="152"/>
        <source>Text color</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="159"/>
        <source>The default color for texts, dimension texts and label texts</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="182"/>
        <source>Lines and arrows</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="188"/>
        <source>If checked, the dimension line is displayed by default</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="191"/>
        <source>Show dimension line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="207"/>
        <source>Line width</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="214"/>
        <source>The default line width</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="243"/>
        <source>Arrow type</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="256"/>
        <source>The default symbol displayed at the ends of dimension lines</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="294"/>
        <source>Arrow size</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="301"/>
        <source>The default arrow size</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="326"/>
        <source>Line and arrow color</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="333"/>
        <source>The default color for lines and arrows</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="356"/>
        <source>Units</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="362"/>
        <source>If checked, a unit symbol is added to dimension texts by default</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="365"/>
        <source>Show unit</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="381"/>
        <source>Unit override</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="394"/>
        <source>The default unit override for dimensions. Enter a unit such as m
or cm, leave blank to use the current unit defined in FreeCAD.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="422"/>
        <source>The default number of decimal places for dimension texts</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="457"/>
        <source>The optional string inserted between the feet and inches values in dimensions</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="473"/>
        <source>Dimension details</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="492"/>
        <source>The default distance the dimension line is extended past the extension lines</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="521"/>
        <source>Extension line length</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="528"/>
        <source>The default length of extension lines. Use 0 for full extension lines. A negative
value defines the gap between the ends of the extension lines and the measured
points. A positive value defines the maximum length of the extension lines. Only
used for linear dimensions.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="560"/>
        <source>The default length of extension lines above the dimension line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="589"/>
        <source>The default space between the dimension line and the dimension text</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="617"/>
        <source>ShapeStrings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="630"/>
        <source>Font files (*.ttf *.otf *.pfb *.TTF *.OTF *.PFB)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="89"/>
        <source>Default height for texts and dimensions</source>
        <translation>預設文字和標註高度</translation>
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
        <translation>尺寸設定</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="124"/>
        <source>Display mode</source>
        <translation>顯示模式</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-drafttexts.ui" line="153"/>
        <source>World</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-drafttexts.ui" line="158"/>
        <source>Screen</source>
        <translation>螢幕</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="161"/>
        <source>Number of decimals</source>
        <translation>小數位數</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="201"/>
        <source>Extension lines size</source>
        <translation>延伸線大小</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="208"/>
        <source>The default size of dimensions extension lines</source>
        <translation>標註延長線預設大小</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="237"/>
        <source>Extension line overshoot</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="244"/>
        <source>The default length of extension line above dimension line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="273"/>
        <source>Dimension line overshoot</source>
        <translation>尺寸線超越量</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="280"/>
        <source>The default distance the dimension line is extended past extension lines</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="309"/>
        <source>Arrows style</source>
        <translation>箭頭樣式</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="323"/>
        <source>Dot</source>
        <translation>點</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="328"/>
        <source>Circle</source>
        <translation>圓</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="333"/>
        <source>Arrow</source>
        <translation>箭頭</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="338"/>
        <source>Tick</source>
        <translation>鉤號</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="343"/>
        <source>Tick-2</source>
        <translation>鉤號-2</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="355"/>
        <source>Arrows size</source>
        <translation>箭頭尺寸</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="362"/>
        <source>The default size of arrows</source>
        <translation>箭頭預設尺寸</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="388"/>
        <source>Text orientation</source>
        <translation>文字方向</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="395"/>
        <source>This is the orientation of the dimension texts when those dimensions are vertical. Default is left, which is the ISO standard.</source>
        <translation>當尺度為垂直時,此為尺度文字方向,預設為左且為ISO標準.</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="405"/>
        <source>Left (ISO standard)</source>
        <translation>左側(ISO標準)</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="410"/>
        <source>Right</source>
        <translation>右視圖</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="422"/>
        <source>Text spacing</source>
        <translation>文字間距</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="429"/>
        <source>The space between the dimension line and the dimension text</source>
        <translation>標註線與文字間的距離</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="455"/>
        <source>Show the unit suffix in dimensions</source>
        <translation>於標註中顯示單位</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="475"/>
        <source>Override unit</source>
        <translation>覆蓋單位</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="495"/>
        <source>By leaving this field blank, the dimension measurements will be shown in the current unit defined in FreeCAD. By indicating a unit here such as m or cm, you can force new dimensions to be shown in that unit.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-drafttexts.ui" line="521"/>
        <source>Feet separator</source>
        <translation>英吋分離器</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-drafttexts.ui" line="541"/>
        <source>Optional string to appear between the feet and inches values in dimensions</source>
        <translation>顯示在尺寸中的英尺和英寸值之間的可選字串</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="513"/>
        <source>ShapeString settings</source>
        <translation>字串造型產生器設定</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="521"/>
        <source>Default ShapeString font file</source>
        <translation>預設字串造型產生器字型檔</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-drafttexts.ui" line="580"/>
        <source>Font files (*.ttf *.otf *.pfb)</source>
        <translation>字型檔(*.ttf *.otf *.pfb)</translation>
    </message>
    <message>
        <location filename="../ui/preferences-drafttexts.ui" line="534"/>
        <source>Select a font file</source>
        <translation>選擇一個字型檔</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="14"/>
        <source>General settings</source>
        <translation>一般設定</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="32"/>
        <source>General Draft Settings</source>
        <translation>一般底圖設定</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="14"/>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="20"/>
        <source>General</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="39"/>
        <source>The number of decimals used in internal coordinate operations (for example 3 = 0.001).
Values between 6 and 8 are usually considered the best trade-off.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="40"/>
        <source>Default working plane</source>
        <translation>預設工作平面</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="73"/>
        <source>The default working plane for new views. If set to &quot;Automatic&quot; the working plane
will automatically align with the current view whenever a command is started.
Additionally it will align to preselected planar faces, or when points on planar
faces are picked during commands.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="112"/>
        <source>If checked, a widget indicating the current working
plane orientation appears when picking points</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="116"/>
        <source>Show working plane tracker</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="129"/>
        <source>If checked, the layers drop-down list also includes groups.
Objects can then automatically be added to groups as well.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="133"/>
        <source>Include groups in layer list</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="149"/>
        <source>Command options</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="155"/>
        <source>If checked, instructions are displayed in the Report view when using Draft commands</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="158"/>
        <source>Show prompts in the Report view</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="171"/>
        <source>If checked, Length input, instead of the X coordinate, will have the initial focus.
This allows to indicate a direction and then type a distance.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="188"/>
        <source>If checked, base objects, instead of created copies, are selected after copying</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="204"/>
        <source>If checked, Draft commands will create Part primitives instead of Draft objects.
Note that this is not fully supported, and many objects will not be editable with
Draft modification commands.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="209"/>
        <source>Create Part primitives if possible</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="222"/>
        <source>If checked, Draft Downgrade and Draft Upgrade will keep face colors.
Only for the splitFaces and makeShell options.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="226"/>
        <source>Keep face colors during downgrade/upgrade</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="239"/>
        <source>If checked, Draft Downgrade and Draft Upgrade will keep face names.
Only for the splitFaces and makeShell options.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="243"/>
        <source>Keep face names during downgrade/upgrade</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="256"/>
        <source>Max. number of editable objects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="263"/>
        <source>The maximum number of objects Draft Edit is allowed to process at the same time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="295"/>
        <source>Edit node pick radius</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="302"/>
        <source>The pick radius of edit nodes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="305"/>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-drafttexts.ui" line="217"/>
        <source> px</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="324"/>
        <source>Label prefix for clones</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="337"/>
        <source>The default prefix added to the label of new clones</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="353"/>
        <source>Construction group label</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="366"/>
        <source>The default label for the construction geometry group</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draft.ui" line="389"/>
        <source>The default color for Draft objects in construction mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="67"/>
        <source>None</source>
        <translation>無</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="72"/>
        <source>XY (Top)</source>
        <translation>XY （上）</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="77"/>
        <source>XZ (Front)</source>
        <translation>XZ (正面)</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="82"/>
        <source>YZ (Side)</source>
        <translation>YZ（側面）</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="94"/>
        <source>Internal precision level</source>
        <translation>內部精度等級</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="120"/>
        <source>The number of decimals in internal coordinates operations (for ex. 3 = 0.001). Values between 6 and 8 are usually considered the best trade-off among FreeCAD users.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="143"/>
        <source>Tolerance</source>
        <translation>公差</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="169"/>
        <source>This is the value used by functions that use a tolerance.
Values with differences below this value will be treated as same. This value will be obsoleted soon so the precision level above controls both.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="194"/>
        <source>If this option is checked, the layers drop-down list will also show groups, allowing you to automatically add objects to groups too.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="197"/>
        <source>Show groups in layers list drop-down button</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="216"/>
        <source>Draft tools options</source>
        <translation>底圖工具選項</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="230"/>
        <source>When drawing lines, set focus on Length instead of X coordinate.
This allows to point the direction and type the distance.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="234"/>
        <source>Set focus on Length instead of X coordinate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="247"/>
        <source>Normally, after copying objects, the copies get selected.
If this option is checked, the base objects will be selected instead.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="251"/>
        <source>Select base objects after copying</source>
        <translation>複製後選定原始物體</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="264"/>
        <source>If this option is set, when creating Draft objects on top of an existing face of another object, the &quot;Support&quot; property of the Draft object will be set to the base object. This was the standard behaviour before FreeCAD 0.19</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="267"/>
        <source>Set the Support property when possible</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="280"/>
        <source>If this is checked, objects will appear as filled by default.
Otherwise, they will appear as wireframe</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="284"/>
        <source>Fill objects with faces whenever possible</source>
        <translation>當可行時將物體以面填滿</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="300"/>
        <source>If this is checked, copy mode will be kept across command,
otherwise commands will always start in no-copy mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="304"/>
        <source>Global copy mode</source>
        <translation>全球複製模式</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="320"/>
        <source>Force Draft Tools to create Part primitives instead of Draft objects.
Note that this is not fully supported, and many object will be not editable with Draft Modifiers.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="324"/>
        <source>Use Part Primitives when available</source>
        <translation>當許可時使用零件圖元</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="341"/>
        <source>Prefix labels of Clones with:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="372"/>
        <source>Construction Geometry</source>
        <translation>建構用幾何圖元</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="380"/>
        <source>Construction group name</source>
        <translation>建構群組名稱</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="387"/>
        <source>This is the default group name for construction geometry</source>
        <translation>這是建構幾何的預設組名稱</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="390"/>
        <source>Construction</source>
        <translation>建構</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="407"/>
        <source>Construction geometry color</source>
        <translation>輔助用幾何色彩</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draft.ui" line="427"/>
        <source>This is the default color for objects being drawn while in construction mode.</source>
        <translation>這是建構模式中繪製物體的預設顏色。</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="14"/>
        <source>Visual settings</source>
        <translation>視覺設定</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="26"/>
        <source>Visual Settings</source>
        <translation>視覺設定</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="34"/>
        <source>Snap symbols style</source>
        <translation>鎖點符號樣式</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="54"/>
        <source>Draft classic style</source>
        <translation>底圖經典風格</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="59"/>
        <source>Bitsnpieces style</source>
        <translation>Bitsnpieces 樣式</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="80"/>
        <source>Color</source>
        <translation>顏色</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="87"/>
        <source>The default color for snap symbols</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="111"/>
        <source>Check this if you want to use the color/linewidth from the toolbar as default</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="114"/>
        <source>Save current color and linewidth across sessions</source>
        <translation>儲存目前色彩及線寬橫跨工作階段</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="131"/>
        <source>If checked, a widget indicating the current working plane orientation appears during drawing operations</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="134"/>
        <source>Show Working Plane tracker</source>
        <translation>顯示工作平面追蹤器</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="151"/>
        <source>Default template sheet</source>
        <translation>預設範本表</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="177"/>
        <source>The default template to use when creating a new drawing sheet</source>
        <translation>建立新圖面時使用的預設模板</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="194"/>
        <source>Alternate SVG patterns location</source>
        <translation>備用 SVG 圖案位置</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="220"/>
        <source>Here you can specify a directory with custom SVG files containing &lt;pattern&gt; definitions to be added to the standard patterns</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="237"/>
        <source>SVG pattern resolution</source>
        <translation>SVG 圖案分辨率</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="257"/>
        <source>The resolution to draw the patterns in. Default value is 128. Higher values give better resolutions, lower values make drawing faster</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="280"/>
        <source>SVG pattern default size</source>
        <translation>SVG 圖案預設大小</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="300"/>
        <source>The default size for SVG patterns</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="326"/>
        <source>Check this if you want to preserve colors of faces while doing downgrade and upgrade (splitFaces and makeShell only)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="329"/>
        <source>Preserve colors of faces during downgrade/upgrade</source>
        <translation>在降級/升級期間保留表面顏色</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="346"/>
        <source>Check this if you want the face names to derive from the originating object name and vice versa while doing downgrade/upgrade (splitFaces and makeShell only)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="349"/>
        <source>Preserve names of faces during downgrade/upgrade</source>
        <translation>在降級/升級期間保留表面名稱</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="367"/>
        <source>Drawing view line definitions</source>
        <translation>工程圖線段定義</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="375"/>
        <source>Dashed line definition</source>
        <translation>虛線(Dash)設定</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="395"/>
        <location filename="../ui/preferences-draftvisual.ui" line="438"/>
        <location filename="../ui/preferences-draftvisual.ui" line="481"/>
        <source>An SVG linestyle definition</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="398"/>
        <source>0.09,0.05</source>
        <translation>0.09,0.05</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="418"/>
        <source>Dashdot line definition</source>
        <translation>虛線(Dashdot)設定</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="441"/>
        <source>0.09,0.05,0.02,0.05</source>
        <translation>0.09,0.05,0.02,0.05</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="461"/>
        <source>Dotted line definition</source>
        <translation>虛線(Dot)設定</translation>
    </message>
    <message>
        <location filename="../ui/preferences-draftvisual.ui" line="484"/>
        <source>0.02,0.02</source>
        <translation>0.02,0.02</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dwg.ui" line="14"/>
        <source>DWG</source>
        <translation>DWG</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dwg.ui" line="26"/>
        <source>DWG conversion</source>
        <translation>DWG 轉換</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dwg.ui" line="34"/>
        <source>Conversion method:</source>
        <translation>轉換方法:</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dwg.ui" line="41"/>
        <source>This is the method FreeCAD will use to convert DWG files to DXF. If &quot;Automatic&quot; is chosen, FreeCAD will try to find one of the following converters in the same order as they are shown here. If FreeCAD is unable to find any, you might need to choose a specific converter and indicate its path here under. Choose the &quot;dwg2dxf&quot; utility if using LibreDWG, &quot;ODAFileConverter&quot; if using the ODA file converter, or the &quot;dwg2dwg&quot; utility if using the pro version of QCAD.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dwg.ui" line="51"/>
        <source>Automatic</source>
        <translation>自動</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dwg.ui" line="56"/>
        <source>LibreDWG</source>
        <translation>LibreDWG</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dwg.ui" line="61"/>
        <source>ODA Converter</source>
        <translation>ODA 轉換器</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dwg.ui" line="66"/>
        <source>QCAD pro</source>
        <translation>QCAD pro</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dwg.ui" line="78"/>
        <source>Path to file converter</source>
        <translation>檔案轉換目錄</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dwg.ui" line="85"/>
        <source>The path to your DWG file converter executable</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dwg.ui" line="100"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-weight:600;&quot;&gt;Note:&lt;/span&gt; DXF options apply to DWG files as well.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-weight:600;&quot;&gt;注意：&lt;/span&gt;DXF選項也套用至DWG檔。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="14"/>
        <source>DXF</source>
        <translation>DXF</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="35"/>
        <source>This preferences dialog will be shown when importing/ exporting DXF files</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="38"/>
        <source>Show this dialog when importing and exporting</source>
        <translation>當匯入及匯出時顯示此對話窗</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="51"/>
        <source>Python importer is used, otherwise the newer C++ is used.
Note: C++ importer is faster, but is not as featureful yet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="55"/>
        <source>Use legacy python importer</source>
        <translation>使用舊有python匯入器</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="71"/>
        <source>Python exporter is used, otherwise the newer C++ is used.
Note: C++ exporter is faster, but is not as featureful yet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="75"/>
        <source>Use legacy python exporter</source>
        <translation>使用傳統python匯出器</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="88"/>
        <source>Automatic update (legacy importer only)</source>
        <translation>自動更新(僅適用於舊的匯入器)</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="96"/>
        <source>Allow FreeCAD to download the Python converter for DXF import and export.
You can also do this manually by installing the &quot;dxf_library&quot; workbench
from the Addon Manager.</source>
        <translation>允許 FreeCAD 下載用於 DXF 匯入和匯出的 Python 轉換器。
您也可以通過附加元件管理器安裝「dxf_library」工作台手動執行此操作。</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="101"/>
        <source>Allow FreeCAD to automatically download and update the DXF libraries</source>
        <translation>允許FreeCAD自動下載並更新DXF函式庫</translation>
    </message>
    <message>
        <location filename="../ui/preferences-oca.ui" line="26"/>
        <location filename="../ui/preferences-dxf.ui" line="119"/>
        <location filename="../ui/preferences-svg.ui" line="26"/>
        <source>Import options</source>
        <translation>匯入選項</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="140"/>
        <source>Note: Not all the options below are used by the new importer yet</source>
        <translation>請注意：並非下列所有選項皆可被新的匯入功能所使用</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="149"/>
        <source>Import</source>
        <translation>匯入</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="156"/>
        <source>If unchecked, texts and mtexts won&apos;t be imported</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="159"/>
        <source>texts and dimensions</source>
        <translation>文字和尺寸</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="172"/>
        <source>If unchecked, points won&apos;t be imported</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="175"/>
        <source>points</source>
        <translation>點</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="188"/>
        <source>If checked, paper space objects will be imported too</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="191"/>
        <source>layouts</source>
        <translation>配置</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="204"/>
        <source>If you want the non-named blocks (beginning with a *) to be imported too</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="207"/>
        <source>*blocks</source>
        <translation>*塊</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="224"/>
        <source>Create</source>
        <translation>建立</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="231"/>
        <source>Only standard Part objects will be created (fastest)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="234"/>
        <source>simple Part shapes</source>
        <translation>簡單零件造型</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="250"/>
        <source>Parametric Draft objects will be created whenever possible</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="253"/>
        <source>Draft objects</source>
        <translation>底圖物體</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="266"/>
        <source>Sketches will be created whenever possible</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="269"/>
        <source>Sketches</source>
        <translation>草圖</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="289"/>
        <source>Scale factor to apply to imported files</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="309"/>
        <source>Scale factor to apply to DXF files on import.
The factor is the conversion between the unit of your DXF file and millimeters.
Example: for files in millimeters: 1, in centimeters: 10,
                             in meters: 1000, in inches: 25.4, in feet: 304.8</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="338"/>
        <source>Colors will be retrieved from the DXF objects whenever possible.
Otherwise default colors will be applied. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="342"/>
        <source>Get original colors from the DXF file</source>
        <translation>從DXF檔中取得原始色彩</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="448"/>
        <source>Import hatch boundaries as wires</source>
        <translation>匯入剖面線邊界為線條</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="469"/>
        <source>Render polylines with width</source>
        <translation>算繪帶有寬度之聚合線</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="570"/>
        <source>Export 3D objects as polyface meshes</source>
        <translation>以聚合面網格匯出3D物體</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="587"/>
        <source>Drawing Views will be exported as blocks.
This might fail for post DXF R12 templates.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="591"/>
        <source>Export Drawing Views as blocks</source>
        <translation>將視圖以圖塊形式匯出</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="614"/>
        <source>Project exported objects along current view direction</source>
        <translation>匯出沿目前視圖方向投影物體</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-dxf.ui" line="342"/>
        <source>Get original colors from the DXF file (legacy importer only)</source>
        <translation>從DXF檔中取得原始色彩(僅傳統輸入)</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="359"/>
        <source>FreeCAD will try to join coincident objects into wires.
Note that this can take a while!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="363"/>
        <source>Join geometry</source>
        <translation>加入幾何形</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="380"/>
        <source>Objects from the same layers will be joined into Draft Blocks,
turning the display faster, but making them less easily editable </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="384"/>
        <source>Group layers into blocks</source>
        <translation>將圖層作為圖塊</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="401"/>
        <source>Imported texts will get the standard Draft Text size,
instead of the size they have in the DXF document</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="405"/>
        <source>Use standard font size for texts</source>
        <translation>文字採用標準尺寸</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="422"/>
        <source>If this is checked, DXF layers will be imported as Draft Layers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="425"/>
        <source>Use Layers</source>
        <translation>使用圖層</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="445"/>
        <source>Hatches will be converted into simple wires</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-dxf.ui" line="448"/>
        <source>Import hatch boundaries as wires (legacy importer only)</source>
        <translation>匯入剖面線邊界為線條(僅傳統輸入)</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="465"/>
        <source>If polylines have a width defined, they will be rendered
as closed wires with correct width</source>
        <translation>若勾選, 當聚合線具有寬度設定時, 其會以正確寬度的封閉線條算繪</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-dxf.ui" line="469"/>
        <source>Render polylines with width (legacy importer only)</source>
        <translation>渲染具有寬度的折線(僅傳統輸入)</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="486"/>
        <source>Ellipse export is poorly supported. Use this to export them as polylines instead.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="489"/>
        <source>Treat ellipses and splines as polylines</source>
        <translation>將橢圓及spline當作聚合線</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="518"/>
        <source>Max Spline Segment:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="528"/>
        <source>Maximum length of each of the polyline segments.
If it is set to &apos;0&apos; the whole spline is treated as a straight segment.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="559"/>
        <location filename="../ui/preferences-svg.ui" line="107"/>
        <source>Export options</source>
        <translation>匯出選項</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="567"/>
        <source>All objects containing faces will be exported as 3D polyfaces</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-dxf.ui" line="570"/>
        <source>Export 3D objects as polyface meshes (legacy exporter only)</source>
        <translation>以聚合面網格匯出3D物體(僅傳統輸出)</translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-dxf.ui" line="587"/>
        <source>TechDraw Views will be exported as blocks.
This might fail for post DXF R12 templates.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-dxf.ui" line="591"/>
        <source>Export TechDraw Views as blocks</source>
        <translation>將 工程圖 視圖匯出為區塊</translation>
    </message>
    <message>
        <location filename="../ui/preferences-dxf.ui" line="611"/>
        <source>Exported objects will be projected to reflect the current view direction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../Resources/ui/preferences-dxf.ui" line="614"/>
        <source>Project exported objects along current view direction (legacy exporter only)</source>
        <translation>匯出沿目前視圖方向投影物體(僅傳統輸出)</translation>
    </message>
    <message>
        <location filename="../ui/preferences-oca.ui" line="14"/>
        <source>OCA</source>
        <translation>OCA</translation>
    </message>
    <message>
        <location filename="../ui/preferences-oca.ui" line="46"/>
        <source>Check this if you want the areas (3D faces) to be imported too.</source>
        <translation>如果你想要一併匯入（3D 面）的面域，請勾選此。</translation>
    </message>
    <message>
        <location filename="../ui/preferences-oca.ui" line="49"/>
        <source>Import OCA areas</source>
        <translation>匯入OCA區域</translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="14"/>
        <source>SVG</source>
        <translation>SVG</translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="40"/>
        <source>Import style</source>
        <translation>匯入樣式</translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="47"/>
        <source>Method chosen for importing SVG object color to FreeCAD</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="60"/>
        <source>None (fastest)</source>
        <translation>無（最快）</translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="65"/>
        <source>Use default color and linewidth</source>
        <translation>使用預設顏色和線寬</translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="70"/>
        <source>Original color and linewidth</source>
        <translation>原有的顏色和線寬</translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="82"/>
        <source>If checked, no units conversion will occur.
One unit in the SVG file will translate as one millimeter. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="86"/>
        <source>Disable units scaling</source>
        <translation>停止單位縮放</translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="121"/>
        <source>Export style</source>
        <translation>匯出格式</translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="128"/>
        <source>Style of SVG file to write when exporting a sketch</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="141"/>
        <source>Translated (for print &amp; display)</source>
        <translation>轉換(用於列印和顯示)</translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="146"/>
        <source>Raw (for CAM)</source>
        <translation>原始檔(用於CAM)</translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="158"/>
        <source>All white lines will appear in black in the SVG for better readability against white backgrounds</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="161"/>
        <source>Translate white line color to black</source>
        <translation>轉換白線色彩為黑色</translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="181"/>
        <source>Max segment length for discretized arcs</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../ui/preferences-svg.ui" line="204"/>
        <source>Versions of Open CASCADE older than version 6.8 don&apos;t support arc projection.
In this case arcs will be discretized into small line segments.
This value is the maximum segment length. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftvisual.ui" line="14"/>
        <source>Visual</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftvisual.ui" line="20"/>
        <source>SVG patterns</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftvisual.ui" line="26"/>
        <source>SVG pattern size</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftvisual.ui" line="39"/>
        <source>The default size for SVG patterns. A higher value results in a denser pattern.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftvisual.ui" line="65"/>
        <source>Additional SVG pattern location</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/Resources/ui/preferences-draftvisual.ui" line="75"/>
        <source>An optional directory with custom SVG files containing
pattern definitions to be added to the standard patterns</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ImportAirfoilDAT</name>
    <message>
        <location filename="../../importAirfoilDAT.py" line="193"/>
        <source>Did not find enough coordinates</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ImportSVG</name>
    <message>
        <location filename="../../importSVG.py" line="1796"/>
        <source>Unknown SVG export style, switching to Translated</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../importSVG.py" line="1816"/>
        <source>The export list contains no object with a valid bounding box</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../../InitGui.py" line="135"/>
        <location filename="../../InitGui.py" line="136"/>
        <location filename="../../InitGui.py" line="137"/>
        <location filename="../../InitGui.py" line="138"/>
        <location filename="../../InitGui.py" line="139"/>
        <source>Draft</source>
        <translation>底圖繪製</translation>
    </message>
    <message>
        <location filename="../../InitGui.py" line="181"/>
        <location filename="../../InitGui.py" line="182"/>
        <location filename="../../InitGui.py" line="183"/>
        <location filename="../../InitGui.py" line="184"/>
        <source>Import-Export</source>
        <translation>匯入-匯出</translation>
    </message>
</context>
<context>
    <name>Workbench</name>
    <message>
        <location filename="../../InitGui.py" line="104"/>
        <source>Draft creation tools</source>
        <translation>底圖建立工具</translation>
    </message>
    <message>
        <location filename="../../InitGui.py" line="107"/>
        <source>Draft annotation tools</source>
        <translation>底圖註釋工具</translation>
    </message>
    <message>
        <location filename="../../InitGui.py" line="110"/>
        <source>Draft modification tools</source>
        <translation>底圖編修工具</translation>
    </message>
    <message>
        <location filename="../../InitGui.py" line="113"/>
        <source>Draft utility tools</source>
        <translation>底圖實用工具</translation>
    </message>
    <message>
        <location filename="../../InitGui.py" line="115"/>
        <source>Draft snap</source>
        <translation>底圖貼齊</translation>
    </message>
    <message>
        <location filename="../../InitGui.py" line="118"/>
        <source>&amp;Drafting</source>
        <translation>底圖繪製(&amp;D)</translation>
    </message>
    <message>
        <location filename="../../InitGui.py" line="121"/>
        <source>&amp;Annotation</source>
        <translation>註釋(&amp;A)</translation>
    </message>
    <message>
        <location filename="../../InitGui.py" line="124"/>
        <source>&amp;Modification</source>
        <translation>編修(&amp;M)</translation>
    </message>
    <message>
        <location filename="../../InitGui.py" line="127"/>
        <source>&amp;Utilities</source>
        <translation>實用工具(&amp;U)</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="50"/>
        <source>Arc tools</source>
        <translation>圓弧工具</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="58"/>
        <source>Bézier tools</source>
        <translation>B-雲形線工具</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="89"/>
        <source>Array tools</source>
        <translation>陣列工具</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snapper.py" line="1477"/>
        <source>Draft Snap</source>
        <translation>底圖貼齊</translation>
    </message>
    <message>
        <source>Draft</source>
        <translation>底圖繪製</translation>
    </message>
</context>
<context>
    <name>draft</name>
    <message>
        <location filename="../../DraftGui.py" line="57"/>
        <location filename="../../DraftGui.py" line="751"/>
        <source>Relative</source>
        <translation>相對關係</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="61"/>
        <location filename="../../DraftGui.py" line="756"/>
        <source>Global</source>
        <translation>全局的</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="66"/>
        <location filename="../../DraftGui.py" line="774"/>
        <location filename="../../DraftGui.py" line="1126"/>
        <source>Continue</source>
        <translation>繼續</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="71"/>
        <location filename="../../DraftGui.py" line="790"/>
        <source>Close</source>
        <translation>關閉</translation>
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
        <translation>複製</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="81"/>
        <source>Subelement mode</source>
        <translation>子元素模式</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="86"/>
        <source>Fill</source>
        <translation>填充</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="91"/>
        <source>Exit</source>
        <translation>結束離開</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="96"/>
        <source>Snap On/Off</source>
        <translation>鎖點模式 開/關</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="101"/>
        <source>Increase snap radius</source>
        <translation>增加捕捉半徑</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="106"/>
        <source>Decrease snap radius</source>
        <translation>減少捕捉半徑</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="111"/>
        <source>Restrict X</source>
        <translation>限定 X</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="116"/>
        <source>Restrict Y</source>
        <translation>限定 Y</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="121"/>
        <source>Restrict Z</source>
        <translation>限定 Z</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="126"/>
        <location filename="../../DraftGui.py" line="796"/>
        <source>Select edge</source>
        <translation>選擇邊緣</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="131"/>
        <source>Add custom snap point</source>
        <translation>新增自訂捕捉點</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="136"/>
        <source>Length mode</source>
        <translation>長度模式</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="141"/>
        <location filename="../../DraftGui.py" line="792"/>
        <source>Wipe</source>
        <translation>擦拭</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="146"/>
        <source>Set Working Plane</source>
        <translation>設定工作平面</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="151"/>
        <source>Cycle snap object</source>
        <translation>循環貼齊物體</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="156"/>
        <source>Toggle near snap on/off</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="330"/>
        <source>Draft Command Bar</source>
        <translation>底圖指令列</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="155"/>
        <source>Undo last segment</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="657"/>
        <source>&amp;Undo</source>
        <translation>復原(&amp;U)</translation>
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
        <translation>無</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="728"/>
        <source>active command:</source>
        <translation>啟動指令：</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="730"/>
        <source>Active Draft command</source>
        <translation>啟動底圖指令</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="731"/>
        <source>X coordinate of next point</source>
        <translation>下一個點的X座標</translation>
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
        <translation>下一個點的Y座標</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="736"/>
        <source>Z coordinate of next point</source>
        <translation>下一個點的Z座標</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="737"/>
        <source>Enter point</source>
        <translation>新增點</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="739"/>
        <source>Enter a new point with the given coordinates</source>
        <translation>以給定座標方式新增點</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="740"/>
        <source>Length</source>
        <translation>間距</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="741"/>
        <location filename="../../draftguitools/gui_trimex.py" line="220"/>
        <source>Angle</source>
        <translation>角度</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="742"/>
        <source>Length of current segment</source>
        <translation>此線段之長度</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="743"/>
        <source>Angle of current segment</source>
        <translation>此線段之角度</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="747"/>
        <source>Check this to lock the current angle</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="748"/>
        <location filename="../../DraftGui.py" line="1108"/>
        <source>Radius</source>
        <translation>半徑</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="749"/>
        <location filename="../../DraftGui.py" line="1109"/>
        <source>Radius of Circle</source>
        <translation>圓的半徑</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="754"/>
        <source>Coordinates relative to last point or to coordinate system origin
if is the first point to set</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="759"/>
        <source>Coordinates relative to global coordinate system.
Uncheck to use working plane coordinate system</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="761"/>
        <source>Filled</source>
        <translation>圓角</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="765"/>
        <source>Check this if the object should appear as filled, otherwise it will appear as wireframe.
Not available if Draft preference option &apos;Use Part Primitives&apos; is enabled</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="767"/>
        <source>Finish</source>
        <translation>結束</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="769"/>
        <source>Finishes the current drawing or editing operation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="772"/>
        <source>If checked, command will not finish until you press the command button again</source>
        <translation>如果選取，指令將不會結束直到再次按下指令按鈕</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="777"/>
        <source>If checked, an OCC-style offset will be performedinstead of the classic offset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="778"/>
        <source>&amp;OCC-style offset</source>
        <translation>&amp;OCC 型式偏移量</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="788"/>
        <source>&amp;Undo (CTRL+Z)</source>
        <translation>復原(&amp;U) (CTRL+Z)</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="802"/>
        <source>If checked, objects will be copied instead of moved. Preferences -&gt; Draft -&gt; Global copy mode to keep this mode in next commands</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="805"/>
        <source>Text string to draw</source>
        <translation>要繪製之文字字串</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="806"/>
        <source>String</source>
        <translation>字串</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="807"/>
        <source>Height of text</source>
        <translation>字高</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="808"/>
        <source>Height</source>
        <translation>高度</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="809"/>
        <source>Intercharacter spacing</source>
        <translation>字元間距</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="810"/>
        <source>Tracking</source>
        <translation>追蹤</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="811"/>
        <source>Full path to font file:</source>
        <translation>字型檔完整路徑：</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="812"/>
        <source>Open a FileChooser for font file</source>
        <translation>選取字型檔</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="836"/>
        <location filename="../../draftguitools/gui_selectplane.py" line="272"/>
        <location filename="../../draftguitools/gui_selectplane.py" line="327"/>
        <location filename="../../draftguitools/gui_selectplane.py" line="530"/>
        <location filename="../../draftguitools/gui_selectplane.py" line="541"/>
        <source>Current working plane</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1197"/>
        <source>Pick Object</source>
        <translation>選取物體</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1203"/>
        <source>Edit</source>
        <translation>編輯</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1253"/>
        <source>Local u0394X</source>
        <translation>區域 u0394X</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1254"/>
        <source>Local u0394Y</source>
        <translation>區域 u0394Y</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1255"/>
        <source>Local u0394Z</source>
        <translation>區域 u0394Z</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1257"/>
        <source>Local X</source>
        <translation>區域X</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1258"/>
        <source>Local Y</source>
        <translation>區域Y</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1259"/>
        <source>Local Z</source>
        <translation>區域Z</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1261"/>
        <source>Global u0394X</source>
        <translation>全域 u0394X</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1262"/>
        <source>Global u0394Y</source>
        <translation>全域 u0394Y</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1263"/>
        <source>Global u0394Z</source>
        <translation>全域 u0394Z</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1265"/>
        <source>Global X</source>
        <translation>全域 X</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1266"/>
        <source>Global Y</source>
        <translation>全域 Y</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1267"/>
        <source>Global Z</source>
        <translation>全域 Z</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1503"/>
        <source>Invalid Size value. Using 200.0.</source>
        <translation>無效值。使用 200.0。</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1511"/>
        <source>Invalid Tracking value. Using 0.</source>
        <translation>追蹤的值無效。使用 0。</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1525"/>
        <source>Please enter a text string.</source>
        <translation>請輸入一段文字.</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1534"/>
        <source>Select a Font file</source>
        <translation>選擇一個字型檔</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1567"/>
        <source>Please enter a font file.</source>
        <translation>請輸入字型檔。</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="654"/>
        <source>If checked, an OCC-style offset will be performed instead of the classic offset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/DraftGui.py" line="565"/>
        <source>OCC-style offset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/DraftGui.py" line="567"/>
        <source>Undo</source>
        <translation>復原</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="789"/>
        <source>Undo the last segment</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="791"/>
        <source>Finishes and closes the current line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="793"/>
        <source>Wipes the existing segments of this line and starts again from the last point</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="794"/>
        <source>Set WP</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="795"/>
        <source>Reorients the working plane on the last segment</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="797"/>
        <source>Selects an existing edge to be measured by this dimension</source>
        <translation>選擇一條現有的邊按此尺寸測量</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="798"/>
        <source>Sides</source>
        <translation>側面</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="799"/>
        <source>Number of sides</source>
        <translation>邊數</translation>
    </message>
    <message>
        <location filename="../../../../../../../Benson/sources/FreeCAD-0.22.0-git/src/Mod/Draft/DraftGui.py" line="581"/>
        <source>If checked, objects will be copied instead of moved</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="803"/>
        <source>Modify subelements</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="804"/>
        <source>If checked, subelements will be modified instead of entire objects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="813"/>
        <source>Create text</source>
        <translation>建立文字</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="814"/>
        <source>Press this button to create the text object, or finish your text with two blank lines</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="837"/>
        <source>Change default style for new objects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="838"/>
        <source>Toggle construction mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="839"/>
        <location filename="../../DraftGui.py" line="2050"/>
        <location filename="../../DraftGui.py" line="2065"/>
        <source>Autogroup off</source>
        <translation>關閉自動群組</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="950"/>
        <source>Line</source>
        <translation>線</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="958"/>
        <source>DWire</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="976"/>
        <source>Circle</source>
        <translation>圓</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="981"/>
        <source>Arc</source>
        <translation>弧</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="986"/>
        <location filename="../../draftguitools/gui_rotate.py" line="286"/>
        <source>Rotate</source>
        <translation>旋轉</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="990"/>
        <source>Point</source>
        <translation>點</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1018"/>
        <source>Label</source>
        <translation>標籤</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="848"/>
        <source>Label type</source>
        <translation>標籤樣式</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1036"/>
        <location filename="../../draftguitools/gui_selectplane.py" line="527"/>
        <location filename="../../draftguitools/gui_offset.py" line="243"/>
        <location filename="../../draftguitools/gui_offset.py" line="260"/>
        <location filename="../../draftguitools/gui_offset.py" line="324"/>
        <source>Offset</source>
        <translation>偏移複製</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1042"/>
        <location filename="../../DraftGui.py" line="1100"/>
        <location filename="../../draftguitools/gui_trimex.py" line="215"/>
        <source>Distance</source>
        <translation>距離</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1043"/>
        <location filename="../../DraftGui.py" line="1101"/>
        <location filename="../../draftguitools/gui_trimex.py" line="217"/>
        <source>Offset distance</source>
        <translation>偏移距離</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="1097"/>
        <source>Trimex</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="965"/>
        <location filename="../../DraftGui.py" line="966"/>
        <location filename="../../DraftGui.py" line="967"/>
        <location filename="../../DraftGui.py" line="969"/>
        <location filename="../../DraftGui.py" line="970"/>
        <location filename="../../DraftGui.py" line="971"/>
        <source>Local {}</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="973"/>
        <location filename="../../DraftGui.py" line="974"/>
        <location filename="../../DraftGui.py" line="975"/>
        <location filename="../../DraftGui.py" line="977"/>
        <location filename="../../DraftGui.py" line="978"/>
        <location filename="../../DraftGui.py" line="979"/>
        <source>Global {}</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="2058"/>
        <source>Autogroup:</source>
        <translation>自動群組:</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="2394"/>
        <source>Faces</source>
        <translation>面</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="2395"/>
        <source>Remove</source>
        <translation>移除</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="2396"/>
        <source>Add</source>
        <translation>新增</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="2397"/>
        <source>Facebinder elements</source>
        <translation>面連接器</translation>
    </message>
    <message>
        <location filename="../../WorkingPlane.py" line="713"/>
        <location filename="../../draftmake/make_sketch.py" line="127"/>
        <location filename="../../draftmake/make_sketch.py" line="139"/>
        <source>All Shapes must be coplanar</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../WorkingPlane.py" line="721"/>
        <source>Selected Shapes must define a plane</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="659"/>
        <location filename="../../WorkingPlane.py" line="821"/>
        <location filename="../../draftguitools/gui_selectplane.py" line="374"/>
        <source>Top</source>
        <translation>上視圖</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="661"/>
        <location filename="../../WorkingPlane.py" line="832"/>
        <location filename="../../draftguitools/gui_selectplane.py" line="386"/>
        <source>Front</source>
        <translation>前視圖</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="663"/>
        <location filename="../../WorkingPlane.py" line="843"/>
        <location filename="../../draftguitools/gui_selectplane.py" line="398"/>
        <source>Side</source>
        <translation>側面</translation>
    </message>
    <message>
        <location filename="../../DraftGui.py" line="665"/>
        <source>Auto</source>
        <translation>自動</translation>
    </message>
    <message>
        <location filename="../../WorkingPlane.py" line="1198"/>
        <location filename="../../WorkingPlane.py" line="1657"/>
        <source>Current working plane:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../WorkingPlane.py" line="1264"/>
        <location filename="../../WorkingPlane.py" line="1289"/>
        <source>Selected shapes do not define a plane</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_selectplane.py" line="539"/>
        <source>Custom</source>
        <translation>自訂</translation>
    </message>
    <message>
        <location filename="../../WorkingPlane.py" line="1616"/>
        <source>No previous working plane</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../WorkingPlane.py" line="1626"/>
        <source>No next working plane</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../WorkingPlane.py" line="1661"/>
        <source>Axes:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../WorkingPlane.py" line="1668"/>
        <source>Position:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../importDWG.py" line="284"/>
        <source>Error during DWG conversion.
Try moving the DWG file to a directory path without spaces and non-english characters,
or try saving to a lower DWG version.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../importDWG.py" line="218"/>
        <location filename="../../importDWG.py" line="290"/>
        <source>Converting:</source>
        <translation>轉換中:</translation>
    </message>
    <message>
        <location filename="../../importDWG.py" line="223"/>
        <source>Conversion successful</source>
        <translation>轉換成功</translation>
    </message>
    <message>
        <location filename="../../importDWG.py" line="302"/>
        <location filename="../../importDWG.py" line="382"/>
        <source>LibreDWG converter not found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../importDWG.py" line="321"/>
        <location filename="../../importDWG.py" line="396"/>
        <source>ODA converter not found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../importDWG.py" line="339"/>
        <location filename="../../importDWG.py" line="407"/>
        <source>QCAD converter not found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../importDWG.py" line="344"/>
        <location filename="../../importDWG.py" line="412"/>
        <source>No suitable external DWG converter has been found.
Please set one manually under menu Edit -&gt; Preferences -&gt; Import/Export -&gt; DWG
For more information see:
https://wiki.freecad.org/Import_Export_Preferences</source>
        <translation type="unfinished"></translation>
    </message>
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
        <translation>此系統並無FreeCAD所需用來匯入/匯出DXF格式之函式庫，可啟動FreeCAD來下載這些函式庫：
1-載入底圖模組
2-選單&gt;編輯&gt;偏好選項&gt;匯入-匯出&gt;DXF&gt;啟動下載
或是手動下載這些函式庫，參照於
https://github.com/yorikvanhavre/Draft-dxf-importer
之說明啟動FreeCAD來下載這些函式庫，並回答確定。</translation>
    </message>
    <message>
        <location filename="../../InitGui.py" line="46"/>
        <source>Draft</source>
        <translation>製圖</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_selectplane.py" line="150"/>
        <source>Pick a face, 3 vertices or a WP Proxy to define the drawing plane</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_selectplane.py" line="274"/>
        <source>Working plane aligned to global placement of</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_selectplane.py" line="523"/>
        <source>Dir</source>
        <translation>方向</translation>
    </message>
    <message>
        <location filename="../../importDWG.py" line="209"/>
        <location filename="../../importDWG.py" line="281"/>
        <source>LibreDWG error</source>
        <translation>LibreDWG 錯誤</translation>
    </message>
    <message>
        <location filename="../../importDWG.py" line="226"/>
        <source>Error during DWG conversion. Try moving the DWG file to a directory path without spaces and non-english characters, or try saving to a lower DWG version.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../importDWG.py" line="229"/>
        <location filename="../../importDWG.py" line="296"/>
        <source>ODA File Converter not found</source>
        <translation>沒找到 ODA 檔案轉換</translation>
    </message>
    <message>
        <location filename="../../importDWG.py" line="242"/>
        <location filename="../../importDWG.py" line="306"/>
        <source>QCAD error</source>
        <translation>QCAD 錯誤</translation>
    </message>
    <message>
        <source>The Draft workbench is used for 2D drafting on a grid</source>
        <translation>製圖工作台用於在網格上進行平面繪圖</translation>
    </message>
    <message>
        <location filename="../../draftutils/gui_utils.py" line="81"/>
        <source>No graphical interface</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/gui_utils.py" line="161"/>
        <source>Unable to insert new object into a scaled part</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/gui_utils.py" line="267"/>
        <source>Symbol not implemented. Using a default symbol.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/gui_utils.py" line="333"/>
        <source>Visibility off; removed from list: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/gui_utils.py" line="603"/>
        <source>image is Null</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/gui_utils.py" line="609"/>
        <source>filename does not exist on the system or in the resource file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/gui_utils.py" line="668"/>
        <source>unable to load texture</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/gui_utils.py" line="738"/>
        <source>Does not have &apos;ViewObject.RootNode&apos;.</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_pointarray.py" line="135"/>
        <source>Wrong input: point object doesn&apos;t have &apos;Geometry&apos;, &apos;Links&apos;, or &apos;Components&apos;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_pointarray.py" line="147"/>
        <location filename="../../draftmake/make_text.py" line="107"/>
        <location filename="../../draftmake/make_label.py" line="215"/>
        <source>Wrong input: must be a placement, a vector, or a rotation.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/init_draft_statusbar.py" line="51"/>
        <location filename="../../draftutils/init_draft_statusbar.py" line="58"/>
        <location filename="../../draftutils/init_draft_statusbar.py" line="65"/>
        <location filename="../../draftutils/init_draft_statusbar.py" line="154"/>
        <source>custom</source>
        <translation>自訂</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_draft_statusbar.py" line="140"/>
        <source>Unable to convert input into a  scale factor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/init_draft_statusbar.py" line="155"/>
        <source>Set custom scale</source>
        <translation>設定自定義縮放比例</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_draft_statusbar.py" line="157"/>
        <source>Set custom annotation scale in format x:x, x=x</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/init_draft_statusbar.py" line="214"/>
        <source>Set the scale used by draft annotation tools</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="650"/>
        <source>Solids:</source>
        <translation>實體:</translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="651"/>
        <source>Faces:</source>
        <translation>面：</translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="652"/>
        <source>Wires:</source>
        <translation>線:</translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="653"/>
        <source>Edges:</source>
        <translation>邊緣:</translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="654"/>
        <source>Vertices:</source>
        <translation>頂點:</translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="658"/>
        <source>Face</source>
        <translation>面</translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="663"/>
        <source>Wire</source>
        <translation>線</translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="695"/>
        <location filename="../../draftutils/utils.py" line="699"/>
        <source>different types</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="709"/>
        <source>Objects have different placements. Distance between the two base points: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="712"/>
        <source>has a different value</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="715"/>
        <source>doesn&apos;t exist in one of the objects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="827"/>
        <source>%s shares a base with %d other objects. Please check if you want to modify this.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="833"/>
        <source>%s cannot be modified because its placement is readonly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="977"/>
        <source>Wrong input: unknown document.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="1055"/>
        <source>This function will be deprecated in </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="1056"/>
        <location filename="../../draftutils/utils.py" line="1059"/>
        <source>Please use </source>
        <translation>請使用 </translation>
    </message>
    <message>
        <location filename="../../draftutils/utils.py" line="1059"/>
        <source>This function will be deprecated. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="169"/>
        <source>Snap Lock</source>
        <translation>自動貼齊定位</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="170"/>
        <source>Snap Endpoint</source>
        <translation>貼齊端點</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="171"/>
        <source>Snap Midpoint</source>
        <translation>貼齊中點</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="172"/>
        <source>Snap Center</source>
        <translation>貼齊中心</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="173"/>
        <source>Snap Angle</source>
        <translation>貼齊角度</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="174"/>
        <source>Snap Intersection</source>
        <translation>貼齊相交</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="175"/>
        <source>Snap Perpendicular</source>
        <translation>貼齊垂直方向</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="176"/>
        <source>Snap Extension</source>
        <translation>延伸貼齊</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="177"/>
        <source>Snap Parallel</source>
        <translation>平行貼齊</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="178"/>
        <source>Snap Special</source>
        <translation>特殊貼齊</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="179"/>
        <source>Snap Near</source>
        <translation>鄰近貼齊</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="180"/>
        <source>Snap Ortho</source>
        <translation>對齊頂部</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="181"/>
        <source>Snap Grid</source>
        <translation>對齊網格</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="182"/>
        <source>Snap WorkingPlane</source>
        <translation>對齊工作平面</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="183"/>
        <source>Snap Dimensions</source>
        <translation>捕捉尺寸</translation>
    </message>
    <message>
        <location filename="../../draftutils/init_tools.py" line="187"/>
        <source>Toggle Draft Grid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/shapestring.py" line="69"/>
        <source>ShapeString: string has no wires</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/draft_annotation.py" line="89"/>
        <location filename="../../draftobjects/draft_annotation.py" line="105"/>
        <source>added view property &apos;ScaleMultiplier&apos;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/draft_annotation.py" line="125"/>
        <location filename="../../draftobjects/draft_annotation.py" line="130"/>
        <source>migrated &apos;DraftText&apos; type to &apos;Text&apos;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="284"/>
        <source>, path object doesn&apos;t have &apos;Edges&apos;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="395"/>
        <location filename="../../draftobjects/patharray.py" line="401"/>
        <location filename="../../draftobjects/patharray.py" line="407"/>
        <source>&apos;PathObj&apos; property will be migrated to &apos;PathObject&apos;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="525"/>
        <source>Cannot calculate path tangent. Copy not aligned.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="541"/>
        <source>Tangent and normal are parallel. Copy not aligned.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="558"/>
        <source>Cannot calculate path normal, using default.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="565"/>
        <source>Cannot calculate path binormal. Copy not aligned.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/patharray.py" line="571"/>
        <source>AlignMode {} is not implemented</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftobjects/pointarray.py" line="145"/>
        <location filename="../../draftobjects/pointarray.py" line="161"/>
        <source>added property &apos;ExtraPlacement&apos;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="151"/>
        <source>Object must be a closed shape</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="153"/>
        <source>No solid object created</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="276"/>
        <source>Faces must be coplanar to be refined</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="435"/>
        <location filename="../../draftfunctions/downgrade.py" line="230"/>
        <source>Upgrade: Unknown force method:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="453"/>
        <source>Found groups: closing each open object inside</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="459"/>
        <source>Found meshes: turning into Part shapes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="467"/>
        <source>Found 1 solidifiable object: solidifying it</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="472"/>
        <source>Found 2 objects: fusing them</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="483"/>
        <source>Found object with several coplanar faces: refine them</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="489"/>
        <source>Found 1 non-parametric objects: draftifying it</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="500"/>
        <source>Found 1 closed sketch object: creating a face from it</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="505"/>
        <source>Found closed wires: creating faces</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="511"/>
        <source>Found several wires or edges: wiring them</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="513"/>
        <location filename="../../draftfunctions/upgrade.py" line="547"/>
        <source>Found several non-treatable objects: creating compound</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="518"/>
        <source>trying: closing it</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="520"/>
        <source>Found 1 open wire: closing it</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="537"/>
        <source>Found 1 object: draftifying it</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="542"/>
        <source>Found points: creating compound</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/upgrade.py" line="550"/>
        <source>Unable to upgrade these objects.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/mirror.py" line="90"/>
        <source>No object given</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/mirror.py" line="94"/>
        <source>The two points are coincident</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/mirror.py" line="113"/>
        <source>mirrored</source>
        <translation>鏡像</translation>
    </message>
    <message>
        <location filename="../../draftfunctions/downgrade.py" line="238"/>
        <source>Found 1 block: exploding it</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/downgrade.py" line="246"/>
        <source>Found 1 multi-solids compound: exploding it</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/downgrade.py" line="253"/>
        <source>Found 1 parametric object: breaking its dependencies</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/downgrade.py" line="261"/>
        <source>Found 2 objects: subtracting them</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/downgrade.py" line="268"/>
        <source>Found several faces: splitting them</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/downgrade.py" line="273"/>
        <source>Found several objects: subtracting them from the first one</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/downgrade.py" line="278"/>
        <source>Found 1 face: extracting its wires</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/downgrade.py" line="284"/>
        <source>Found only wires: extracting their edges</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftfunctions/downgrade.py" line="288"/>
        <source>No more downgrade possible</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_circulararray.py" line="122"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="83"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="95"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="290"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="481"/>
        <source>Circular array</source>
        <translation>圓形陣列</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_circulararray.py" line="144"/>
        <source>Wrong input: must be a number or quantity.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_circulararray.py" line="154"/>
        <location filename="../../draftmake/make_polararray.py" line="112"/>
        <location filename="../../draftmake/make_orthoarray.py" line="119"/>
        <source>Wrong input: must be an integer number.</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="173"/>
        <source>This function is deprecated. Do not use this function directly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="174"/>
        <source>Use one of &apos;make_linear_dimension&apos;, or &apos;make_linear_dimension_obj&apos;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="316"/>
        <location filename="../../draftmake/make_label.py" line="230"/>
        <source>Wrong input: object must not be a list.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="327"/>
        <location filename="../../draftmake/make_dimension.py" line="452"/>
        <source>Wrong input: object doesn&apos;t have a &apos;Shape&apos; to measure.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="331"/>
        <source>Wrong input: object doesn&apos;t have at least one element in &apos;Vertexes&apos; to use for measuring.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="338"/>
        <location filename="../../draftmake/make_dimension.py" line="463"/>
        <source>Wrong input: must be an integer.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="343"/>
        <source>i1: values below 1 are not allowed; will be set to 1.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="347"/>
        <location filename="../../draftmake/make_dimension.py" line="363"/>
        <source>Wrong input: vertex not in object.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="359"/>
        <source>i2: values below 1 are not allowed; will be set to the last vertex in the object.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="456"/>
        <source>Wrong input: object doesn&apos;t have at least one element in &apos;Edges&apos; to use for measuring.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="468"/>
        <source>index: values below 1 are not allowed; will be set to 1.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="472"/>
        <source>Wrong input: index doesn&apos;t correspond to an edge in the object.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="476"/>
        <source>Wrong input: index doesn&apos;t correspond to a circular edge.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="483"/>
        <location filename="../../draftmake/make_dimension.py" line="487"/>
        <source>Wrong input: must be a string, &apos;radius&apos; or &apos;diameter&apos;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_dimension.py" line="579"/>
        <location filename="../../draftmake/make_dimension.py" line="586"/>
        <source>Wrong input: must be a list with two angles.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_patharray.py" line="213"/>
        <location filename="../../draftmake/make_label.py" line="251"/>
        <source>Wrong input: must be a list or tuple of strings, or a single string.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_patharray.py" line="246"/>
        <source>Wrong input: must be &apos;Original&apos;, &apos;Frenet&apos;, or &apos;Tangent&apos;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_label.py" line="263"/>
        <source>Wrong input: subelement not in object.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_label.py" line="272"/>
        <source>Wrong input: label_type must be a string.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_label.py" line="277"/>
        <source>Wrong input: label_type must be one of the following: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_label.py" line="300"/>
        <location filename="../../draftmake/make_label.py" line="304"/>
        <source>Wrong input: must be a string, &apos;Horizontal&apos;, &apos;Vertical&apos;, or &apos;Custom&apos;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_label.py" line="320"/>
        <source>Wrong input: must be a list of at least two vectors.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_label.py" line="353"/>
        <source>Direction is not &apos;Custom&apos;; points won&apos;t be used.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_label.py" line="380"/>
        <source>Wrong input: must be a list of two elements. For example, [object, &apos;Edge1&apos;].</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_text.py" line="91"/>
        <location filename="../../draftmake/make_text.py" line="96"/>
        <location filename="../../draftmake/make_label.py" line="286"/>
        <location filename="../../draftmake/make_label.py" line="291"/>
        <source>Wrong input: must be a list of strings or a single string.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_polararray.py" line="94"/>
        <location filename="../../drafttaskpanels/task_polararray.py" line="95"/>
        <location filename="../../drafttaskpanels/task_polararray.py" line="258"/>
        <location filename="../../drafttaskpanels/task_polararray.py" line="434"/>
        <source>Polar array</source>
        <translation>極座標陣列</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_polararray.py" line="119"/>
        <location filename="../../draftmake/make_layer.py" line="201"/>
        <location filename="../../draftmake/make_patharray.py" line="191"/>
        <location filename="../../draftmake/make_patharray.py" line="360"/>
        <location filename="../../draftmake/make_orthoarray.py" line="151"/>
        <location filename="../../draftmake/make_label.py" line="313"/>
        <source>Wrong input: must be a number.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_layer.py" line="69"/>
        <source>Layers</source>
        <translation>圖層</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_layer.py" line="145"/>
        <location filename="../../draftmake/make_layer.py" line="162"/>
        <location filename="../../draftguitools/gui_layers.py" line="47"/>
        <source>Layer</source>
        <translation>圖層</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_layer.py" line="157"/>
        <source>Wrong input: it must be a string.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_layer.py" line="167"/>
        <location filename="../../draftmake/make_layer.py" line="171"/>
        <location filename="../../draftmake/make_layer.py" line="184"/>
        <location filename="../../draftmake/make_layer.py" line="188"/>
        <source>Wrong input: must be a tuple of three floats 0.0 to 1.0.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_layer.py" line="208"/>
        <location filename="../../draftmake/make_layer.py" line="212"/>
        <source>Wrong input: must be &apos;Solid&apos;, &apos;Dashed&apos;, &apos;Dotted&apos;, or &apos;Dashdot&apos;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_layer.py" line="220"/>
        <source>Wrong input: must be a number between 0 and 100.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_orthoarray.py" line="60"/>
        <source>Internal orthogonal array</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_orthoarray.py" line="87"/>
        <source>Wrong input: must be a number or vector.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_orthoarray.py" line="92"/>
        <location filename="../../draftmake/make_orthoarray.py" line="95"/>
        <location filename="../../draftmake/make_orthoarray.py" line="98"/>
        <source>Input: single value expanded to vector.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_orthoarray.py" line="123"/>
        <location filename="../../draftmake/make_orthoarray.py" line="126"/>
        <location filename="../../draftmake/make_orthoarray.py" line="129"/>
        <source>Input: number of elements must be at least 1. It is set to 1.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_orthoarray.py" line="275"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="83"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="95"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="269"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="396"/>
        <source>Orthogonal array</source>
        <translation>正交陣列</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_orthoarray.py" line="350"/>
        <source>Orthogonal array 2D</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_orthoarray.py" line="424"/>
        <source>Rectangular array</source>
        <translation>矩形陣列</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_orthoarray.py" line="501"/>
        <source>Rectangular array 2D</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="125"/>
        <location filename="../../draftmake/make_arc_3points.py" line="130"/>
        <source>Points:</source>
        <translation>點:</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="126"/>
        <location filename="../../draftmake/make_arc_3points.py" line="131"/>
        <source>Wrong input: must be list or tuple of three points exactly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="138"/>
        <source>Placement:</source>
        <translation>放置位置:</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="139"/>
        <source>Wrong input: incorrect type of placement.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="153"/>
        <source>Wrong input: incorrect type of points.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="159"/>
        <source>Cannot generate shape:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="166"/>
        <source>Radius:</source>
        <translation>半徑：</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="167"/>
        <source>Center:</source>
        <translation>中心：</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="170"/>
        <source>Create primitive object</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="193"/>
        <location filename="../../draftmake/make_arc_3points.py" line="204"/>
        <source>Final placement:</source>
        <translation>最終放置位置:</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="195"/>
        <source>Face: True</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="197"/>
        <source>Support:</source>
        <translation>支撐:</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_arc_3points.py" line="198"/>
        <source>Map mode:</source>
        <translation>地圖模式:</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_sketch.py" line="104"/>
        <source>No shape found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_sketch.py" line="111"/>
        <source>All Shapes must be planar</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_fillet.py" line="58"/>
        <location filename="../../draftmake/make_fillet.py" line="143"/>
        <location filename="../../draftmake/make_fillet.py" line="144"/>
        <location filename="../../draftmake/make_fillet.py" line="145"/>
        <source>length:</source>
        <translation>長度:</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_fillet.py" line="132"/>
        <source>Two elements are needed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_fillet.py" line="139"/>
        <source>Radius is too large</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftmake/make_fillet.py" line="143"/>
        <location filename="../../draftmake/make_fillet.py" line="144"/>
        <location filename="../../draftmake/make_fillet.py" line="145"/>
        <source>Segment</source>
        <translation>分割</translation>
    </message>
    <message>
        <location filename="../../draftmake/make_fillet.py" line="165"/>
        <source>Removed original objects.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_scale.py" line="87"/>
        <source>Select an object to scale</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_scale.py" line="108"/>
        <source>Pick base point</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_scale.py" line="135"/>
        <source>Pick reference distance from base point</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_scale.py" line="206"/>
        <location filename="../../draftguitools/gui_scale.py" line="236"/>
        <location filename="../../draftguitools/gui_scale.py" line="359"/>
        <source>Scale</source>
        <translation>縮放</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_scale.py" line="209"/>
        <source>Some subelements could not be scaled.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_scale.py" line="339"/>
        <source>Unable to scale object:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_scale.py" line="343"/>
        <source>Unable to scale objects:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_scale.py" line="346"/>
        <source>This object type cannot be scaled directly. Please use the clone method.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_scale.py" line="407"/>
        <source>Pick new distance from base point</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_edit_sketcher_objects.py" line="63"/>
        <source>Sketch is too complex to edit: it is suggested to use sketcher default editor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_labels.py" line="80"/>
        <source>Pick target point</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_labels.py" line="157"/>
        <source>Create Label</source>
        <translation>建立標籤</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_labels.py" line="191"/>
        <location filename="../../draftguitools/gui_labels.py" line="218"/>
        <source>Pick endpoint of leader line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_labels.py" line="201"/>
        <location filename="../../draftguitools/gui_labels.py" line="228"/>
        <source>Pick text position</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_styles.py" line="75"/>
        <source>Change Style</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_texts.py" line="77"/>
        <source>Pick location point</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_texts.py" line="121"/>
        <source>Create Text</source>
        <translation>建立文字</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_grid.py" line="51"/>
        <source>Toggle grid</source>
        <translation>切換格線</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_facebinders.py" line="71"/>
        <source>Select faces from existing objects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_mirror.py" line="73"/>
        <source>Select an object to mirror</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_mirror.py" line="92"/>
        <source>Pick start point of mirror line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_mirror.py" line="122"/>
        <source>Mirror</source>
        <translation>鏡像</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_mirror.py" line="177"/>
        <location filename="../../draftguitools/gui_mirror.py" line="203"/>
        <source>Pick end point of mirror line</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_polygons.py" line="78"/>
        <location filename="../../draftguitools/gui_arcs.py" line="88"/>
        <source>Pick center point</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_polygons.py" line="189"/>
        <location filename="../../draftguitools/gui_polygons.py" line="200"/>
        <location filename="../../draftguitools/gui_polygons.py" line="260"/>
        <location filename="../../draftguitools/gui_arcs.py" line="254"/>
        <location filename="../../draftguitools/gui_arcs.py" line="270"/>
        <location filename="../../draftguitools/gui_arcs.py" line="410"/>
        <source>Pick radius</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_polygons.py" line="224"/>
        <source>Create Polygon (Part)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_polygons.py" line="243"/>
        <source>Create Polygon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="277"/>
        <location filename="../../draftguitools/gui_arcs.py" line="278"/>
        <location filename="../../draftguitools/gui_arcs.py" line="446"/>
        <location filename="../../draftguitools/gui_arcs.py" line="447"/>
        <source>Start angle</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="283"/>
        <location filename="../../draftguitools/gui_arcs.py" line="452"/>
        <source>Pick start angle</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="285"/>
        <location filename="../../draftguitools/gui_arcs.py" line="286"/>
        <location filename="../../draftguitools/gui_arcs.py" line="454"/>
        <location filename="../../draftguitools/gui_arcs.py" line="455"/>
        <source>Aperture angle</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="291"/>
        <source>Pick aperture</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="317"/>
        <source>Create Circle (Part)</source>
        <translation>建立圓形（零件）</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="335"/>
        <source>Create Circle</source>
        <translation>建立圓形</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="369"/>
        <source>Create Arc (Part)</source>
        <translation>建立弧線（零件）</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="389"/>
        <source>Create Arc</source>
        <translation>建立弧線</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="466"/>
        <source>Pick aperture angle</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_arcs.py" line="509"/>
        <location filename="../../draftguitools/gui_arcs.py" line="551"/>
        <source>Arc by 3 points</source>
        <translation>3點建立弧</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_ellipses.py" line="71"/>
        <location filename="../../draftguitools/gui_lines.py" line="83"/>
        <location filename="../../draftguitools/gui_dimensions.py" line="122"/>
        <location filename="../../draftguitools/gui_rectangles.py" line="69"/>
        <source>Pick first point</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_ellipses.py" line="121"/>
        <location filename="../../draftguitools/gui_ellipses.py" line="138"/>
        <source>Create Ellipse</source>
        <translation>建立橢圓</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_ellipses.py" line="196"/>
        <location filename="../../draftguitools/gui_rectangles.py" line="202"/>
        <source>Pick opposite point</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_lines.py" line="163"/>
        <source>Create Line</source>
        <translation>建立直線</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_lines.py" line="185"/>
        <source>Create Wire</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_lines.py" line="330"/>
        <source>Unable to create a Wire from selected objects</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_lines.py" line="352"/>
        <source>Convert to Wire</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_shapestrings.py" line="85"/>
        <location filename="../../draftguitools/gui_shapestrings.py" line="98"/>
        <source>Pick ShapeString location point</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_shapestring.py" line="56"/>
        <source>ShapeString</source>
        <translation>字串造型產生器</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_shapestring.py" line="70"/>
        <source>Default</source>
        <translation>預設</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_shapestring.py" line="176"/>
        <location filename="../../draftguitools/gui_shapestrings.py" line="133"/>
        <source>Create ShapeString</source>
        <translation>建立字串造型產生器</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_edit.py" line="305"/>
        <source>Select a Draft object to edit</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_edit.py" line="558"/>
        <source>No edit point found for selected object</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_edit.py" line="811"/>
        <source>Too many objects selected, max number set to:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_edit.py" line="819"/>
        <source>: this object is not editable</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_join.py" line="76"/>
        <source>Select an object to join</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_join.py" line="99"/>
        <source>Join lines</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_join.py" line="110"/>
        <source>Selection:</source>
        <translation>選擇：</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_lineslope.py" line="64"/>
        <source>Change slope</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_trimex.py" line="94"/>
        <source>Select objects to trim or extend</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_trimex.py" line="173"/>
        <location filename="../../draftguitools/gui_offset.py" line="143"/>
        <source>Pick distance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_trimex.py" line="222"/>
        <source>Offset angle</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_trimex.py" line="483"/>
        <source>Unable to trim these objects, only Draft wires and arcs are supported.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_trimex.py" line="488"/>
        <source>Unable to trim these objects, too many wires</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_trimex.py" line="505"/>
        <source>These objects don&apos;t intersect.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_trimex.py" line="508"/>
        <source>Too many intersection points.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_draft2sketch.py" line="66"/>
        <source>Select an object to convert.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_draft2sketch.py" line="99"/>
        <source>Convert to Sketch</source>
        <translation>轉換為草圖</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_draft2sketch.py" line="114"/>
        <source>Convert to Draft</source>
        <translation>轉換為底圖</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_draft2sketch.py" line="143"/>
        <source>Convert Draft/Sketch</source>
        <translation>轉換 底圖/草圖</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_pointarray.py" line="104"/>
        <source>Please select exactly two objects, the base object and the point object, before calling this command.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_pointarray.py" line="122"/>
        <source>Point array</source>
        <translation>點陣列</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_subelements.py" line="108"/>
        <source>Select an object to edit</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_clone.py" line="79"/>
        <source>Select an object to clone</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_dimensions.py" line="212"/>
        <location filename="../../draftguitools/gui_dimensions.py" line="247"/>
        <location filename="../../draftguitools/gui_dimensions.py" line="261"/>
        <location filename="../../draftguitools/gui_dimensions.py" line="294"/>
        <source>Create Dimension</source>
        <translation>建立標註</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_dimensions.py" line="309"/>
        <source>Create Dimension (radial)</source>
        <translation>建立尺寸（圓弧）</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_dimensions.py" line="508"/>
        <source>Edge too short!</source>
        <translation>邊緣過短！</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_dimensions.py" line="518"/>
        <source>Edges don&apos;t intersect!</source>
        <translation>邊緣未相交!</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_stretch.py" line="75"/>
        <source>Select an object to stretch</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_stretch.py" line="127"/>
        <source>Pick first point of selection rectangle</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_stretch.py" line="164"/>
        <source>Pick opposite point of selection rectangle</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_stretch.py" line="173"/>
        <source>Pick start point of displacement</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_stretch.py" line="236"/>
        <source>Pick end point of displacement</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_stretch.py" line="448"/>
        <source>Turning one Rectangle into a Wire</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_stretch.py" line="477"/>
        <source>Stretch</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_patharray.py" line="102"/>
        <location filename="../../draftguitools/gui_pathtwistedarray.py" line="79"/>
        <source>Please select exactly two objects, the base object and the path object, before calling this command.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_patharray.py" line="143"/>
        <source>Path array</source>
        <translation>路徑陣列</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_pathtwistedarray.py" line="101"/>
        <source>Path twisted array</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="132"/>
        <location filename="../../draftguitools/gui_beziers.py" line="332"/>
        <source>Bézier curve has been closed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="140"/>
        <location filename="../../draftguitools/gui_beziers.py" line="368"/>
        <location filename="../../draftguitools/gui_splines.py" line="131"/>
        <source>Last point has been removed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="153"/>
        <location filename="../../draftguitools/gui_splines.py" line="147"/>
        <source>Pick next point, or finish (A) or close (O)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="211"/>
        <location filename="../../draftguitools/gui_beziers.py" line="451"/>
        <source>Create BezCurve</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="376"/>
        <source>Click and drag to define next knot</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_beziers.py" line="382"/>
        <source>Click and drag to define next knot, or finish (A) or close (O)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snapper.py" line="1543"/>
        <source>(ON)</source>
        <translation>(開啟)</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snapper.py" line="1546"/>
        <source>(OFF)</source>
        <translation>(關閉)</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_downgrade.py" line="67"/>
        <location filename="../../draftguitools/gui_upgrade.py" line="67"/>
        <source>Select an object to upgrade</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_downgrade.py" line="85"/>
        <source>Downgrade</source>
        <translation>降級</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_splines.py" line="120"/>
        <source>Spline has been closed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_splines.py" line="183"/>
        <source>Create B-spline</source>
        <translation>建立B雲形線</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_rectangles.py" line="125"/>
        <source>Create Plane</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_rectangles.py" line="142"/>
        <source>Create Rectangle</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="76"/>
        <source>Fillet radius</source>
        <translation>圓角半徑</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="79"/>
        <source>Radius of fillet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="107"/>
        <source>Enter radius.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="126"/>
        <source>Delete original objects:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="131"/>
        <source>Chamfer mode:</source>
        <translation>倒角模式:</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="148"/>
        <source>Two elements needed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="155"/>
        <source>Test object</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="156"/>
        <source>Test object removed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="158"/>
        <source>Fillet cannot be created</source>
        <translation>無法建立倒角</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_fillets.py" line="188"/>
        <source>Create fillet</source>
        <translation>建立圓角</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="65"/>
        <source>Add to group</source>
        <translation>新增到群組</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="68"/>
        <source>Ungroup</source>
        <translation>取消群組</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="70"/>
        <source>Add new group</source>
        <translation>新增群組</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="159"/>
        <source>Select group</source>
        <translation>選擇群組</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="193"/>
        <source>No new selection. You must select non-empty groups or objects inside groups.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="203"/>
        <source>Autogroup</source>
        <translation>自動群組</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="250"/>
        <source>Add new Layer</source>
        <translation>新增圖層</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="304"/>
        <source>Add to construction group</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="355"/>
        <source>Add a new group with a given name</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="383"/>
        <source>Add group</source>
        <translation>新增群組</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="385"/>
        <source>Group name</source>
        <translation>群組名稱</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_groups.py" line="392"/>
        <source>Group</source>
        <translation>群組</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_edit_draft_objects.py" line="92"/>
        <location filename="../../draftguitools/gui_edit_draft_objects.py" line="569"/>
        <source>This object does not support possible coincident points, please try again.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_edit_draft_objects.py" line="164"/>
        <location filename="../../draftguitools/gui_edit_draft_objects.py" line="608"/>
        <source>Active object must have more than two points/nodes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_edit_draft_objects.py" line="722"/>
        <source>Selection is not a Knot</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_edit_draft_objects.py" line="749"/>
        <source>Endpoint of BezCurve can&apos;t be smoothed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_drawing.py" line="77"/>
        <source>The Drawing Workbench is obsolete since 0.17, consider using the TechDraw Workbench instead.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_drawing.py" line="81"/>
        <location filename="../../draftguitools/gui_shape2dview.py" line="68"/>
        <source>Select an object to project</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_upgrade.py" line="85"/>
        <source>Upgrade</source>
        <translation>升級</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="126"/>
        <source>Main toggle snap</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="157"/>
        <source>Midpoint snap</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="187"/>
        <source>Perpendicular snap</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="217"/>
        <source>Grid snap</source>
        <translation>格點快選</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="247"/>
        <source>Intersection snap</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="277"/>
        <source>Parallel snap</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="307"/>
        <source>Endpoint snap</source>
        <translation>貼齊端點</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="338"/>
        <source>Angle snap (30 and 45 degrees)</source>
        <translation>貼齊角度（30或45度角）</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="368"/>
        <source>Arc center snap</source>
        <translation>貼齊圓弧中心</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="398"/>
        <source>Edge extension snap</source>
        <translation>貼齊延伸邊緣</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="428"/>
        <source>Near snap</source>
        <translation>鄰近貼齊</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="459"/>
        <source>Orthogonal snap</source>
        <translation>垂直貼齊</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="489"/>
        <source>Special point snap</source>
        <translation>貼齊特殊點</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="520"/>
        <source>Dimension display</source>
        <translation>尺寸顯示</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="553"/>
        <source>Working plane snap</source>
        <translation>貼齊工作平面</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_snaps.py" line="583"/>
        <source>Show snap toolbar</source>
        <translation>顯示貼齊工具列</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_move.py" line="81"/>
        <source>Select an object to move</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_move.py" line="103"/>
        <source>Pick start point</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_move.py" line="162"/>
        <location filename="../../draftguitools/gui_move.py" line="308"/>
        <source>Pick end point</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_move.py" line="210"/>
        <source>Move</source>
        <translation>移動</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_move.py" line="213"/>
        <location filename="../../draftguitools/gui_rotate.py" line="289"/>
        <source>Some subelements could not be moved.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_dimension_ops.py" line="55"/>
        <source>Flip dimension</source>
        <translation>旋轉尺寸</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_togglemodes.py" line="73"/>
        <source>No active Draft Toolbar.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_togglemodes.py" line="93"/>
        <source>Construction mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_togglemodes.py" line="125"/>
        <source>Continue mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_togglemodes.py" line="159"/>
        <source>Toggle display mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="68"/>
        <source>Annotation style editor</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="290"/>
        <source>Open styles file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="292"/>
        <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="314"/>
        <source>JSON file (*.json)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="312"/>
        <source>Save styles file</source>
        <translation>儲存樣式檔案</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_heal.py" line="51"/>
        <source>Heal</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_points.py" line="134"/>
        <location filename="../../draftguitools/gui_points.py" line="147"/>
        <source>Create Point</source>
        <translation>建立點</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_offset.py" line="76"/>
        <source>Select an object to offset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_offset.py" line="82"/>
        <source>Offset only works on one object at a time.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_offset.py" line="92"/>
        <source>Cannot offset this object type</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_offset.py" line="123"/>
        <source>Offset of Bezier curves is currently not supported</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_rotate.py" line="79"/>
        <source>Select an object to rotate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_rotate.py" line="99"/>
        <source>Pick rotation center</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_rotate.py" line="193"/>
        <location filename="../../draftguitools/gui_rotate.py" line="396"/>
        <source>Base angle</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_rotate.py" line="194"/>
        <location filename="../../draftguitools/gui_rotate.py" line="397"/>
        <source>The base angle you wish to start the rotation from</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_rotate.py" line="199"/>
        <location filename="../../draftguitools/gui_rotate.py" line="400"/>
        <source>Pick base angle</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_rotate.py" line="205"/>
        <location filename="../../draftguitools/gui_rotate.py" line="409"/>
        <source>Rotation</source>
        <translation>旋轉</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_rotate.py" line="206"/>
        <location filename="../../draftguitools/gui_rotate.py" line="410"/>
        <source>The amount of rotation you wish to perform.
The final angle will be the base angle plus this amount.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_rotate.py" line="213"/>
        <location filename="../../draftguitools/gui_rotate.py" line="418"/>
        <source>Pick rotation angle</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_shape2dview.py" line="109"/>
        <source>Create 2D view</source>
        <translation>建立2D視景</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_array_simple.py" line="77"/>
        <source>Select an object to array</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_array_simple.py" line="101"/>
        <source>Array</source>
        <translation>矩陣</translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_split.py" line="64"/>
        <source>Click anywhere on a line to split it.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftguitools/gui_split.py" line="106"/>
        <source>Split line</source>
        <translation>拆分線段</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="83"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="83"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="83"/>
        <source>Task panel:</source>
        <translation>工作面板：</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="187"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="208"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="201"/>
        <source>At least one element must be selected.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="194"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="219"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="212"/>
        <source>Selection is not suitable for array.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="195"/>
        <location filename="../../drafttaskpanels/task_polararray.py" line="327"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="220"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="372"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="213"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="375"/>
        <source>Object:</source>
        <translation>物體:</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="199"/>
        <source>Number of elements must be at least 2.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="203"/>
        <source>The angle is above 360 degrees. It is set to this value to proceed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="206"/>
        <source>The angle is below -360 degrees. It is set to this value to proceed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="286"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="331"/>
        <source>Center reset:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="296"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="341"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="344"/>
        <source>Fuse:</source>
        <translation>融合:</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="310"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="355"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="358"/>
        <source>Create Link array:</source>
        <translation>建立連結陣列：</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="328"/>
        <source>Number of elements:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="329"/>
        <source>Polar angle:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="331"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="378"/>
        <source>Center of rotation:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_polararray.py" line="434"/>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="481"/>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="396"/>
        <source>Aborted:</source>
        <translation>已中止:</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="205"/>
        <source>Number of elements must be at least 1.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="316"/>
        <source>Interval X reset:</source>
        <translation>間距 X 重置:</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="325"/>
        <source>Interval Y reset:</source>
        <translation>間距 Y 重置:</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="334"/>
        <source>Interval Z reset:</source>
        <translation>間距 Z 重置:</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="376"/>
        <source>Number of X elements:</source>
        <translation>X 方向的複製數:</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="378"/>
        <source>Interval X:</source>
        <translation>間距 X:</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="381"/>
        <source>Number of Y elements:</source>
        <translation>Y 方向的複製數:</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="383"/>
        <source>Interval Y:</source>
        <translation>間距 Y:</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="386"/>
        <source>Number of Z elements:</source>
        <translation>Z 方向的複製數:</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_orthoarray.py" line="388"/>
        <source>Interval Z:</source>
        <translation>間距 Z:</translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="212"/>
        <source>Number of layers must be at least 2.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="224"/>
        <source>Radial distance is zero. Resulting array may not look correct.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="226"/>
        <source>Radial distance is negative. It is made positive to proceed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="230"/>
        <source>Tangential distance cannot be zero.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="233"/>
        <source>Tangential distance is negative. It is made positive to proceed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="373"/>
        <source>Radial distance:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="374"/>
        <source>Tangential distance:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="375"/>
        <source>Number of circular layers:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../drafttaskpanels/task_circulararray.py" line="376"/>
        <source>Symmetry parameter:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="361"/>
        <source>Activate this layer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="367"/>
        <source>Select layer contents</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="405"/>
        <location filename="../../draftviewproviders/view_layer.py" line="421"/>
        <source>Merge layer duplicates</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="410"/>
        <location filename="../../draftviewproviders/view_layer.py" line="469"/>
        <source>Add new layer</source>
        <translation>新增圖層</translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="454"/>
        <source>Relabeling layer:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_layer.py" line="458"/>
        <source>Merging layer:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../draftviewproviders/view_base.py" line="402"/>
        <source>Please load the Draft Workbench to enable editing this object</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>importOCA</name>
    <message>
        <location filename="../../importOCA.py" line="360"/>
        <source>OCA error: couldn&apos;t determine character encoding</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../importOCA.py" line="445"/>
        <source>OCA: found no data to export</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../importOCA.py" line="490"/>
        <source>successfully exported</source>
        <translation>成功匯出</translation>
    </message>
</context>
</TS>
