<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ja" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../draftobjects/wire.py" line="49"/>
      <source>The vertices of the wire</source>
      <translation>ワイヤーの頂点</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="53"/>
      <source>If the wire is closed or not</source>
      <translation>ワイヤーが閉じているかどうか</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="57"/>
      <source>The base object is the wire, it's formed from 2 objects</source>
      <translation>ベースオブジェクトは2つのオブジェクトから形成されたワイヤーです。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="61"/>
      <source>The tool object is the wire, it's formed from 2 objects</source>
      <translation>ツールオブジェクトは2つのオブジェクトから形成されたワイヤーです。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="65"/>
      <source>The start point of this line</source>
      <translation>この線の始点</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="69"/>
      <source>The end point of this line</source>
      <translation>この線の終点</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="73"/>
      <source>The length of this line</source>
      <translation>この線の長さ</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="52"/>
      <location filename="../../draftobjects/polygon.py" line="60"/>
      <location filename="../../draftobjects/wire.py" line="77"/>
      <source>Radius to use to fillet the corners</source>
      <translation>角のフィレットで使用する半径</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="55"/>
      <location filename="../../draftobjects/polygon.py" line="64"/>
      <location filename="../../draftobjects/wire.py" line="81"/>
      <source>Size of the chamfer to give to the corners</source>
      <translation>角の面取りのサイズ</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="85"/>
      <source>Create a face if this object is closed</source>
      <translation>このオブジェクトが閉じている場合、面を作成</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wire.py" line="89"/>
      <source>The number of subdivisions of each edge</source>
      <translation>各エッジの再分割数</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="67"/>
      <location filename="../../draftobjects/circle.py" line="62"/>
      <location filename="../../draftobjects/polygon.py" line="72"/>
      <location filename="../../draftobjects/bspline.py" line="57"/>
      <location filename="../../draftobjects/bezcurve.py" line="70"/>
      <location filename="../../draftobjects/wire.py" line="93"/>
      <source>The area of this object</source>
      <translation>このオブジェクトの面積</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="65"/>
      <source>The position of the tip of the leader line.
This point can be decorated with an arrow or another symbol.</source>
      <translation>引き出し線の先端の位置。
この点は矢印または別の記号で装飾できます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="79"/>
      <source>Object, and optionally subelement, whose properties will be displayed
as 'Text', depending on 'Label Type'.

'Target' won't be used if 'Label Type' is set to 'Custom'.</source>
      <translation>「Label Type」に従って「Text」として表示されるプロパティーを持つオブジェクトやオプションのサブ要素。

「Label Type」が「Custom」に設定されている場合は「Target」は使用されません。</translation>
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
      <translation>引き出し線を定義する点のリスト。通常は3点のリストです。

最初の点はテキスト位置、つまり「Placement」、
最後の点は線の先端、つまり「Target Point」になります。
中間の点は「Straight Direction」と「Straight Distance」の値の
選択・設定に従って自動で計算されます。

「Straight Direction」が「Custom」に設定されている場合は
「Points」プロパティーには任意の点のリストを設定できます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="124"/>
      <source>The direction of the straight segment of the leader line.

If 'Custom' is chosen, the points of the leader can be specified by
assigning a custom list to the 'Points' attribute.</source>
      <translation>引き出し線の直線部分の方向。

「Custom」 が選択されている場合、「Points」属性にカスタムリストを
設定することで引き出し線の点を指定できます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="142"/>
      <source>The length of the straight segment of the leader line.

This is an oriented distance; if it is negative, the line will be drawn
to the left or below the 'Text', otherwise to the right or above it,
depending on the value of 'Straight Direction'.</source>
      <translation>引き出し線の直線部分の長さ。

方向を持つ距離です。「Straight Direction」の値に従って、
マイナスの場合は線がテキストの左または下に描かれ、
それ以外の場合は右または上に描かれます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="156"/>
      <source>The placement of the 'Text' element in 3D space</source>
      <translation>3D空間での「テキスト」要素の配置</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="166"/>
      <source>The text to display when 'Label Type' is set to 'Custom'</source>
      <translation>「Label Type」が「Custom」に設定されている場合に表示するテキスト</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="181"/>
      <source>The text displayed by this label.

This property is read-only, as the final text depends on 'Label Type',
and the object defined in 'Target'.
The 'Custom Text' is displayed only if 'Label Type' is set to 'Custom'.</source>
      <translation>このラベルによって表示されるテキスト

このプロパティーは読み取り専用で、「Label Type」、
「Target」に定義されたオブジェクトによって最終的なテキストが決まります。
「Label Type」が「Custom」に設定されている場合のみ「Custom Text」が表示されます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/label.py" line="216"/>
      <source>The type of information displayed by this label.

If 'Custom' is chosen, the contents of 'Custom Text' will be used.
For other types, the string will be calculated automatically from the object defined in 'Target'.
'Tag' and 'Material' only work for objects that have these properties, like Arch objects.

For 'Position', 'Length', and 'Area' these properties will be extracted from the main object in 'Target',
or from the subelement 'VertexN', 'EdgeN', or 'FaceN', respectively, if it is specified.</source>
      <translation>このラベルによって表示される情報の種類

「Custom」が選択されている場合、「Custom Text」の内容が使用されます。
他の種類では「Target」で定義されるオブジェクトから自動計算される文字列になります。
「Tag」や「Material」は Arch オブジェクトなど、そのプロパティーを持つオブジェクトでのみ動作します。

「位置」「長さ」「面積」のプロパティーは「Target」のメイン・オブジェクト、または
指定された場合はサブ要素「VertexN」「EdgeN」「FaceN」からそれぞれ抽出されます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="46"/>
      <source>The base object used by this object</source>
      <translation>このオブジェクトで使用されているベースオブジェクト</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="49"/>
      <source>The PAT file used by this object</source>
      <translation>このオブジェクトで使用されるPATファイル</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="52"/>
      <source>The pattern name used by this object</source>
      <translation>このオブジェクトで使用されているパターン名</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="55"/>
      <source>The pattern scale used by this object</source>
      <translation>このオブジェクトで使用されるパターンスケール</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="58"/>
      <source>The pattern rotation used by this object</source>
      <translation>このオブジェクトで使用されるパターンの回転</translation>
    </message>
    <message>
      <location filename="../../draftobjects/hatch.py" line="61"/>
      <source>If set to False, hatch is applied as is to the faces, without translation (this might give wrong results for non-XY faces)</source>
      <translation>False に設定すると、ハッチは平行移動せずにそのまま面に適用されます (非XY面では間違った結果になる可能性があります)。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="66"/>
      <source>The linked object</source>
      <translation>リンクされたオブジェクト</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="73"/>
      <source>Projection direction</source>
      <translation>投影方向</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="80"/>
      <source>The width of the lines inside this object</source>
      <translation>このオブジェクト内部の線の幅</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="88"/>
      <source>The size of the texts inside this object</source>
      <translation>このオブジェクト内部のテキストの幅</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="96"/>
      <source>The spacing between lines of text</source>
      <translation>テキストの行間隔</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="103"/>
      <source>The color of the projected objects</source>
      <translation>投影オブジェクトの色</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="110"/>
      <source>Shape Fill Style</source>
      <translation>シェイプの塗りつぶしのスタイル</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="118"/>
      <source>Line Style</source>
      <translation>線のスタイル</translation>
    </message>
    <message>
      <location filename="../../draftobjects/drawingview.py" line="127"/>
      <source>If checked, source objects are displayed regardless of being visible in the 3D model</source>
      <translation>チェックされている場合、ソースオブジェクトが3Dモデル内で表示されているかどうかに関わらず表示されます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/circle.py" line="45"/>
      <source>Start angle of the arc</source>
      <translation>円弧の開始角度</translation>
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
      <translation>円の半径</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="58"/>
      <location filename="../../draftobjects/circle.py" line="58"/>
      <location filename="../../draftobjects/polygon.py" line="68"/>
      <location filename="../../draftobjects/ellipse.py" line="58"/>
      <source>Create a face</source>
      <translation>面を作成</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="46"/>
      <source>Text string</source>
      <translation>テキスト文字列</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="49"/>
      <source>Font file name</source>
      <translation>フォントファイル名</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="52"/>
      <source>Height of text</source>
      <translation>テキストの高さ</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="55"/>
      <source>Inter-character spacing</source>
      <translation>文字間隔</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="58"/>
      <source>Fill letters with faces</source>
      <translation type="unfinished">Fill letters with faces</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="86"/>
      <source>The base object that will be duplicated.</source>
      <translation type="unfinished">The base object that will be duplicated.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="93"/>
      <location filename="../../draftobjects/patharray.py" line="177"/>
      <source>The object along which the copies will be distributed. It must contain 'Edges'.</source>
      <translation>コピーがそれに沿って配置されるオブジェクト。「エッジ」を含んでいる必要があります。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="100"/>
      <source>Number of copies to create.</source>
      <translation>作成するコピーの数。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pathtwistedarray.py" line="107"/>
      <source>Rotation factor of the twisted array.</source>
      <translation type="unfinished">Rotation factor of the twisted array.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="320"/>
      <location filename="../../draftobjects/pathtwistedarray.py" line="114"/>
      <location filename="../../draftobjects/pointarray.py" line="112"/>
      <location filename="../../draftobjects/patharray.py" line="208"/>
      <source>Show the individual array elements (only for Link arrays)</source>
      <translation>個々の整列要素を表示 (リンク整列のみ)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="83"/>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="82"/>
      <source>General scaling factor that affects the annotation consistently
because it scales the text, and the line decorations, if any,
in the same proportion.</source>
      <translation>注釈に一貫した効果を及ぼす全般的な拡大縮小率であり、
同じ比率でテキストや線の装飾を拡大縮小します。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="93"/>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="100"/>
      <source>Annotation style to apply to this object.
When using a saved style some of the view properties will become read-only;
they will only be editable by changing the style through the 'Annotation style editor' tool.</source>
      <translation>このオブジェクトに提要する注釈スタイル。
保存されたスタイルを使用するとビュープロパティーの一部が読み取り専用になります。
「注釈スタイルエディター」ツールからスタイルを変更すると編集できるようになります。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draftlink.py" line="99"/>
      <source>Force sync pattern placements even when array elements are expanded</source>
      <translation type="unfinished">Force sync pattern placements even when array elements are expanded</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draftlink.py" line="112"/>
      <source>Show the individual array elements</source>
      <translation>個々の整列要素を表示</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="46"/>
      <source>The objects included in this clone</source>
      <translation>このクローンに含まれるオブジェクト</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="51"/>
      <source>The scale factor of this clone</source>
      <translation>このクローンの拡大縮小率</translation>
    </message>
    <message>
      <location filename="../../draftobjects/clone.py" line="57"/>
      <source>If Clones includes several objects,
set True for fusion or False for compound</source>
      <translation>クローンに複数のオブジェクトが含まれている場合、
Trueに設定すると結合、Falseに設定するとコンパウドを作成。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="46"/>
      <source>The points of the B-spline</source>
      <translation>B-スプラインの点</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="50"/>
      <source>If the B-spline is closed or not</source>
      <translation>B-スプラインが閉じているかどうか</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="54"/>
      <source>Create a face if this spline is closed</source>
      <translation>このスプラインが閉じている場合、面を作成</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="67"/>
      <source>Parameterization factor</source>
      <translation>パラメーター化係数</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="57"/>
      <source>The base object this 2D view must represent</source>
      <translation>この2Dビューが表現する必要のあるベースオブジェクト</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="62"/>
      <source>The projection vector of this object</source>
      <translation>このオブジェクトの投影ベクトル</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="68"/>
      <source>The way the viewed object must be projected</source>
      <translation>表示オブジェクトの投影方向</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="75"/>
      <source>The indices of the faces to be projected in Individual Faces mode</source>
      <translation>個別面モードで投影される面のインデックス</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="80"/>
      <source>Show hidden lines</source>
      <translation>隠線を表示</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="86"/>
      <source>Fuse wall and structure objects of same type and material</source>
      <translation>壁と構造物を同じ種類と材質に融合</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="91"/>
      <source>Tessellate Ellipses and B-splines into line segments</source>
      <translation>楕円、B-スプラインを線分に分割</translation>
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
      <translation type="unfinished">If this is True, this object will include only visible objects</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="117"/>
      <source>A list of exclusion points. Any edge touching any of those points will not be drawn.</source>
      <translation type="unfinished">A list of exclusion points. Any edge touching any of those points will not be drawn.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shape2dview.py" line="122"/>
      <source>If this is True, only solid geometry is handled. This overrides the base object's Only Solids property</source>
      <translation type="unfinished">If this is True, only solid geometry is handled. This overrides the base object's Only Solids property</translation>
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
      <translation>X位置</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="48"/>
      <source>Y Location</source>
      <translation>Y位置</translation>
    </message>
    <message>
      <location filename="../../draftobjects/point.py" line="51"/>
      <source>Z Location</source>
      <translation>Z位置</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="46"/>
      <source>Length of the rectangle</source>
      <translation>四角形の長さ</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="49"/>
      <source>Height of the rectangle</source>
      <translation>四角形の高さ</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="61"/>
      <source>Horizontal subdivisions of this rectangle</source>
      <translation>この四角形の水平方向の細分割数</translation>
    </message>
    <message>
      <location filename="../../draftobjects/rectangle.py" line="64"/>
      <source>Vertical subdivisions of this rectangle</source>
      <translation>この四角形の垂直方向の細分割数</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="43"/>
      <source>Linked faces</source>
      <translation>リンクされた面</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="46"/>
      <source>Specifies if splitter lines must be removed</source>
      <translation>分割線を削除する必要があるかどうかを指定</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="49"/>
      <source>An optional extrusion value to be applied to all faces</source>
      <translation>すべての面に適用するオプションの押し出し値</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="52"/>
      <source>An optional offset value to be applied to all faces</source>
      <translation>すべての面に適用するオプションのオフセット値</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="55"/>
      <source>This specifies if the shapes sew</source>
      <translation>シェイプの縫い合わせを行うかどうかを指定</translation>
    </message>
    <message>
      <location filename="../../draftobjects/facebinder.py" line="58"/>
      <source>The area of the faces of this Facebinder</source>
      <translation>このフェイスバインダーの面積</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="48"/>
      <source>Number of faces</source>
      <translation>面の数</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="52"/>
      <source>Radius of the control circle</source>
      <translation>制御円の半径</translation>
    </message>
    <message>
      <location filename="../../draftobjects/polygon.py" line="56"/>
      <source>How the polygon must be drawn from the control circle</source>
      <translation>制御円から多角形を描く方法</translation>
    </message>
    <message>
      <location filename="../../draftobjects/block.py" line="43"/>
      <source>The components of this block</source>
      <translation>このブロックのコンポーネント</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="47"/>
      <source>The start point of this line.</source>
      <translation>この線の始点.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="55"/>
      <source>The end point of this line.</source>
      <translation>この線の終点.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="63"/>
      <source>The length of this line.</source>
      <translation>この線の長さ.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/fillet.py" line="71"/>
      <source>Radius to use to fillet the corner.</source>
      <translation>角のフィレット半径.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/wpproxy.py" line="42"/>
      <source>The placement of this object</source>
      <translation>このオブジェクトの配置</translation>
    </message>
    <message>
      <location filename="../../draftobjects/layer.py" line="59"/>
      <source>The objects that are part of this layer</source>
      <translation>このレイヤーの一部であるオブジェクト</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="137"/>
      <source>The normal direction of the text of the dimension</source>
      <translation>この寸法のテキストの法線方向</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="150"/>
      <source>The object measured by this dimension object</source>
      <translation>この寸法オブジェクトで測定するオブジェクト</translation>
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
      <translation>寸法線、またはその延長線が通過する点。

- 直線寸法の場合、このプロパティーは寸法線が計測オブジェクトにどれだけ近づくかを制御します。
- 半径寸法の場合、計測半径、計測直径を表示する寸法線の方向を制御します。
- 角度寸法の場合、計測角度を表示する寸法円弧の半径を制御します。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="233"/>
      <source>Starting point of the dimension line.

If it is a radius dimension it will be the center of the arc.
If it is a diameter dimension it will be a point that lies on the arc.</source>
      <translation>寸法線の始点。

半径寸法の場合、円弧の中心点になります。
直径寸法の場合、円弧上の1点になります。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="247"/>
      <source>Ending point of the dimension line.

If it is a radius or diameter dimension
it will be a point that lies on the arc.</source>
      <translation>寸法線の終点。

半径寸法、直径寸法の場合、円弧上の1点になります。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="259"/>
      <source>The direction of the dimension line.
If this remains '(0,0,0)', the direction will be calculated automatically.</source>
      <translation>寸法線の方向。
(0,0,0) のままにした場合、方向は自動計算されます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="276"/>
      <source>The value of the measurement.

This property is read-only because the value is calculated
from the 'Start' and 'End' properties.

If the 'Linked Geometry' is an arc or circle, this 'Distance'
is the radius or diameter, depending on the 'Diameter' property.</source>
      <translation>計測値。

この値は「Start」「End」プロパティーから計算されるため、このプロパティーは読み取り専用です。

「Linked Geometry」が円弧や円の場合、「Diameter」プロパティーに依存して「Distance」は半径または直径になります。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="287"/>
      <source>When measuring circular arcs, it determines whether to display
the radius or the diameter value</source>
      <translation>円弧を計測する際に、半径、直径のどちらを表示するかを決定します。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="507"/>
      <source>Starting angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
      <translation>寸法線の開始角度 (円弧)。
円弧は反時計回りに描画されます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="518"/>
      <source>Ending angle of the dimension line (circular arc).
The arc is drawn counter-clockwise.</source>
      <translation>寸法線の終了角度 (円弧)。
円弧は反時計回りに描画されます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="533"/>
      <source>The center point of the dimension line, which is a circular arc.

This is normally the point where two line segments, or their extensions
intersect, resulting in the measured 'Angle' between them.</source>
      <translation>円弧の寸法線の中心点。

通常、2つの線分、またはそれらの延長線が交差する点で、その間の測定角度が計測結果となります。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/dimension.py" line="547"/>
      <source>The value of the measurement.

This property is read-only because the value is calculated from
the 'First Angle' and 'Last Angle' properties.</source>
      <translation>計測値。

値は「First Angle」と「Last Angle」プロパティーから計算されるため、このプロパティーは読み取り専用です。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="45"/>
      <source>Start angle of the elliptical arc</source>
      <translation>楕円弧の開始角度</translation>
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
      <translation>楕円の短半径</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="55"/>
      <source>Major radius of the ellipse</source>
      <translation>楕円の長半径</translation>
    </message>
    <message>
      <location filename="../../draftobjects/ellipse.py" line="61"/>
      <source>Area of this object</source>
      <translation>このオブジェクトの面積</translation>
    </message>
    <message>
      <location filename="../../draftobjects/text.py" line="54"/>
      <source>The placement of the base point of the first line</source>
      <translation>最初の線の基点の位置</translation>
    </message>
    <message>
      <location filename="../../draftobjects/text.py" line="66"/>
      <source>The text displayed by this object.
It is a list of strings; each element in the list will be displayed in its own line.</source>
      <translation>このオブジェクトによって表示されるテキスト。
文字列のリストで、リスト内の各要素がそれぞれの行に表示されます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="82"/>
      <location filename="../../draftobjects/patharray.py" line="169"/>
      <source>The base object that will be duplicated</source>
      <translation>ベースオブジェクトが重複しています。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="192"/>
      <source>List of connected edges in the 'Path Object'.
If these are present, the copies will be created along these subelements only.
Leave this property empty to create copies along the entire 'Path Object'.</source>
      <translation>「Path Object」の繋がったエッジのリスト。
設定されている場合はそれらサブ要素のみに沿ってコピーが作成されます。
「Path Object」全体に沿ってコピーを作成する場合はこのプロパティーを空欄のままにしてください。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="200"/>
      <source>Number of copies to create</source>
      <translation>作成するコピーの数:</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="219"/>
      <source>Additional translation that will be applied to each copy.
This is useful to adjust for the difference between shape centre and shape reference point.</source>
      <translation>各コピーに適用される追加の移動。
シェイプの中心と基準点の差を調整するのに便利です。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="227"/>
      <source>Alignment vector for 'Tangent' mode</source>
      <translation>「Tangent」モードでの配置ベクトル</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="235"/>
      <source>Force use of 'Vertical Vector' as local Z direction when using 'Original' or 'Tangent' alignment mode</source>
      <translation>「Original」または「Tangent」配置モードを使用する際に「Vertical Vector」をローカルZ方向として強制的に使用</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="243"/>
      <source>Direction of the local Z axis when 'Force Vertical' is true</source>
      <translation>「Force Vertical」がTrueの場合のローカルZ軸方向</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="251"/>
      <source>Method to orient the copies along the path.
- Original: X is curve tangent, Y is normal, and Z is the cross product.
- Frenet: aligns the object following the local coordinate system along the path.
- Tangent: similar to 'Original' but the local X axis is pre-aligned to 'Tangent Vector'.

To get better results with 'Original' or 'Tangent' you may have to set 'Force Vertical' to true.</source>
      <translation>パスに沿ったコピーの向き指定方法
- Original: Xは曲線接線、Yは法線、Zは外積の方向。
- Frenet: パスに沿ったローカル座標系に追従するようオブジェクトを配置。
- Tangent: 「Original」と同様ですがローカルX軸が「Tangent Vector」に前もって合わせられます。

「Original」「Tangent」でより適切な結果を得るには「Force Vertical」をTrueに設定しておくと良いかもしれません。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="262"/>
      <source>Orient the copies along the path depending on the 'Align Mode'.
Otherwise the copies will have the same orientation as the original Base object.</source>
      <translation>コピーを「Align Mode」に応じた向きでパスに沿って配置します。
設定されていない場合、コピーは元のベース・オブジェクトと同じ方向になります。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="46"/>
      <source>The points of the Bezier curve</source>
      <translation>ベジエ曲線の点</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="50"/>
      <source>The degree of the Bezier function</source>
      <translation>ベジエ関数の次数</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="54"/>
      <source>Continuity</source>
      <translation>続行</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="58"/>
      <source>If the Bezier curve should be closed or not</source>
      <translation>ベジエ曲線を閉じるかどうか</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="62"/>
      <source>Create a face if this curve is closed</source>
      <translation>この曲線が閉じている場合、面を作成</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bezcurve.py" line="66"/>
      <source>The length of this object</source>
      <translation>このオブジェクトの長さ</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="98"/>
      <source>The type of array to create.
- Ortho: places the copies in the direction of the global X, Y, Z axes.
- Polar: places the copies along a circular arc, up to a specified angle, and with certain orientation defined by a center and an axis.
- Circular: places the copies in concentric circular layers around the base object.</source>
      <translation>作成する整列のタイプ。
- Ortho: コピーをグローバルなX、Y、Zの軸方向に配置
- Polar: 指定された角度まで、中心と軸によって定義された方向に、円弧に沿ってコピーを配置
- Circular: ベースオブジェクトを囲む同心球上にコピーを配置</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="109"/>
      <source>Specifies if the copies should be fused together if they touch each other (slower)</source>
      <translation>互いに接触した場合にコピーを結合するかどうかを指定 (処理が遅くなります)</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="122"/>
      <source>Number of copies in X direction</source>
      <translation>X方向のコピー数</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="131"/>
      <source>Number of copies in Y direction</source>
      <translation>Y方向のコピー数</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="140"/>
      <source>Number of copies in Z direction</source>
      <translation>Z方向のコピー数</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="150"/>
      <source>Distance and orientation of intervals in X direction</source>
      <translation>X方向の区間の距離と向き</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="160"/>
      <source>Distance and orientation of intervals in Y direction</source>
      <translation>Y方向の区間の距離と向き</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="170"/>
      <source>Distance and orientation of intervals in Z direction</source>
      <translation>Z方向の区間の距離と向き</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="185"/>
      <source>The axis direction around which the elements in a polar or a circular array will be created</source>
      <translation>軸周整列または円形整列の要素が作成される軸方向</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="196"/>
      <source>Center point for polar and circular arrays.
The 'Axis' passes through this point.</source>
      <translation>軸周整列、円形配列の中心点。
軸がこの点を通ります。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="215"/>
      <source>The axis object that overrides the value of 'Axis' and 'Center', for example, a datum line.
Its placement, position and rotation, will be used when creating polar and circular arrays.
Leave this property empty to be able to set 'Axis' and 'Center' manually.</source>
      <translation>「Axis」「Center」の値を上書きする、データム線などの軸オブジェクト。
その配置、位置、回転量が軸周配列、円形配列の作成時に使用されます。
「Axis」「Center」を手作業で設定できるようにするにはこのプロパティーを空欄のままにしてください。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="228"/>
      <source>Number of copies in the polar direction</source>
      <translation>軸周方向のコピー数</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="238"/>
      <source>Distance and orientation of intervals in 'Axis' direction</source>
      <translation>「軸」方向の区間の距離と向き</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="247"/>
      <source>Angle to cover with copies</source>
      <translation>コピーを敷きつめる角度</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="260"/>
      <source>Distance between circular layers</source>
      <translation>円形レイヤー間の距離</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="270"/>
      <source>Distance between copies in the same circular layer</source>
      <translation>同じ円形レイヤー内のコピー間の距離</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="280"/>
      <source>Number of circular layers. The 'Base' object counts as one layer.</source>
      <translation>円形レイヤー数。「Base」オブジェクトは1つのレイヤーとしてカウントされます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="291"/>
      <source>A parameter that determines how many symmetry planes the circular array will have.</source>
      <translation type="unfinished">A parameter that determines how many symmetry planes the circular array will have.</translation>
    </message>
    <message>
      <location filename="../../draftobjects/array.py" line="309"/>
      <source>Total number of elements in the array.
This property is read-only, as the number depends on the parameters of the array.</source>
      <translation>整列内の要素の合計数。
整列のパラメーターに依存するためこのプロパティは読み取り専用です。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="79"/>
      <source>Base object that will be duplicated</source>
      <translation>ベースオブジェクトが重複しています。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="87"/>
      <source>Object containing points used to distribute the base object, for example, a sketch or a Part compound.
The sketch or compound must contain at least one explicit point or vertex object.</source>
      <translation>スケッチやパート・コンパウドなど、ベース・オブジェクトを配置するために使用される点を含むオブジェクト。
スケッチまたはコンパウンドは、明示的な点・頂点オブジェクトを少なくとも1つ含んでいる必要があります。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="95"/>
      <source>Total number of elements in the array.
This property is read-only, as the number depends on the points contained within 'Point Object'.</source>
      <translation>整列内の要素の合計数。
「Point Object」に含まれる点に依存する数値であるためこのプロパティーは読み取り専用です。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="104"/>
      <location filename="../../draftobjects/pointarray.py" line="140"/>
      <source>Additional placement, shift and rotation, that will be applied to each copy</source>
      <translation>各コピーに適用される追加の配置、平行移動、回転。</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="60"/>
      <location filename="../../draftviewproviders/view_label.py" line="74"/>
      <source>The size of the text</source>
      <translation>テキストのサイズ</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="69"/>
      <location filename="../../draftviewproviders/view_label.py" line="83"/>
      <source>The font of the text</source>
      <translation>テキストのフォント</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="78"/>
      <location filename="../../draftviewproviders/view_label.py" line="92"/>
      <location filename="../../draftviewproviders/view_label.py" line="119"/>
      <source>The vertical alignment of the text</source>
      <translation>テキストの垂直方向の位置</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="87"/>
      <location filename="../../draftviewproviders/view_label.py" line="102"/>
      <source>Text color</source>
      <translation>テキストの色</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_text.py" line="95"/>
      <location filename="../../draftviewproviders/view_label.py" line="128"/>
      <source>Line spacing (relative to font size)</source>
      <translation>行間隔(フォント サイズに対する相対値)</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="111"/>
      <source>The maximum number of characters on each line of the text box</source>
      <translation>テキストボックスの各行の最大文字数</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="139"/>
      <source>The size of the arrow</source>
      <translation>矢印のサイズ</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="148"/>
      <source>The type of arrow of this label</source>
      <translation>このラベルの矢印の種類</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="159"/>
      <source>The type of frame around the text of this object</source>
      <translation>このオブジェクトのテキストを囲む枠の種類</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_label.py" line="168"/>
      <source>Display a leader line or not</source>
      <translation>引き出し線を表示するかどうか</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="115"/>
      <location filename="../../draftviewproviders/view_label.py" line="177"/>
      <source>Line width</source>
      <translation>ライン幅</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_draft_annotation.py" line="122"/>
      <location filename="../../draftviewproviders/view_label.py" line="186"/>
      <source>Line color</source>
      <translation>線の色</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="137"/>
      <source>Font name</source>
      <translation>フォント名</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="146"/>
      <source>Font size</source>
      <translation>フォントサイズ</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="155"/>
      <source>Spacing between text and dimension line</source>
      <translation>テキストと寸法線との間隔</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="164"/>
      <source>Rotate the dimension text 180 degrees</source>
      <translation>寸法テキストを180度回転</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="174"/>
      <source>Text Position.
Leave '(0,0,0)' for automatic position</source>
      <translation>テキスト位置。
自動位置の場合は「(0,0,0)」のままにします。</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="185"/>
      <source>Text override.
Write '$dim' so that it is replaced by the dimension length.</source>
      <translation>上書きテキス。
「$dim」と書き込むと、寸法長さで置き換えられます。</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="196"/>
      <source>The number of decimals to show</source>
      <translation>表示する小数点以下の桁数</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="205"/>
      <source>Show the unit suffix</source>
      <translation>単位表記を表示</translation>
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
      <translation>矢印のサイズ</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="239"/>
      <source>Arrow type</source>
      <translation>矢印のタイプ</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="249"/>
      <source>Rotate the dimension arrows 180 degrees</source>
      <translation>寸法矢印を180度回転</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="260"/>
      <source>The distance the dimension line is extended
past the extension lines</source>
      <translation>寸法線が寸法補助線を超えて伸びる距離</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="269"/>
      <source>Length of the extension lines</source>
      <translation>寸法補助線の長さ</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="279"/>
      <source>Length of the extension line
beyond the dimension line</source>
      <translation>寸法線を越える寸法補助線の長さ</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_dimension.py" line="288"/>
      <source>Shows the dimension line and arrows</source>
      <translation>寸法線と矢印を表示</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="67"/>
      <source>If it is true, the objects contained within this layer will adopt the line color of the layer</source>
      <translation>Trueの場合、このレイヤーに含まれるオブジェクトにはレイヤーの線の色が適用されます。</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="78"/>
      <source>If it is true, the objects contained within this layer will adopt the shape color of the layer</source>
      <translation>True の場合、このレイヤー内に含まれるオブジェクトはレイヤーの形状色を採用します</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="89"/>
      <source>If it is true, the print color will be used when objects in this layer are placed on a TechDraw page</source>
      <translation>True の場合、このレイヤー内のオブジェクトが TechDraw ページに配置された場合に印刷色が使用されます。</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="103"/>
      <source>The line color of the objects contained within this layer</source>
      <translation>このレイヤーに含まれるオブジェクトの線の色</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="117"/>
      <source>The shape color of the objects contained within this layer</source>
      <translation>このレイヤーに含まれるオブジェクトのシェイプの色</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="131"/>
      <source>The line width of the objects contained within this layer</source>
      <translation>このレイヤーに含まれるオブジェクトの線の幅</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="143"/>
      <source>The draw style of the objects contained within this layer</source>
      <translation>このレイヤーに含まれるオブジェクトの描画スタイル</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="154"/>
      <source>The transparency of the objects contained within this layer</source>
      <translation>このレイヤーに含まれるオブジェクトの透明度</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="165"/>
      <source>The line color of the objects contained within this layer, when used on a TechDraw page</source>
      <translation>TechDraw ページで使用する場合の、このレイヤーに含まれるオブジェクトの線の色</translation>
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
      <translation>注釈スタイルエディター</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="20"/>
      <source>Style name</source>
      <translation>スタイル名</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="38"/>
      <source>The name of your style. Existing style names can be edited.</source>
      <translation>スタイルの名前です。既存のスタイル名は編集できます。</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="50"/>
      <source>Add new...</source>
      <translation>新規追加...</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="79"/>
      <source>Renames the selected style</source>
      <translation>選択されたスタイルの名前を変更</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="82"/>
      <source>Rename</source>
      <translation>名前の変更</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="110"/>
      <source>Deletes the selected style</source>
      <translation>選択したスタイルを削除</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="113"/>
      <source>Delete</source>
      <translation>削除</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="120"/>
      <source>Import styles from json file</source>
      <translation>jsonファイルからスタイルをインポート</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="130"/>
      <source>Export styles to json file</source>
      <translation>jsonファイルへスタイルをエクスポート</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="164"/>
      <source>Text</source>
      <translation>テキスト</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="170"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="192"/>
      <source>The font to use for texts and dimensions</source>
      <translation>テキストと寸法に用いられるフォント</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="173"/>
      <source>Font name</source>
      <translation>フォント名</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="199"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="209"/>
      <source>Font size in the system units</source>
      <translation>システム単位のフォントサイズ</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="202"/>
      <source>Font size</source>
      <translation>フォントサイズ</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="219"/>
      <source>Line spacing in system units</source>
      <translation>システム単位の行間隔</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="222"/>
      <source>Line spacing</source>
      <translation>行間</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="239"/>
      <source>Units</source>
      <translation>単位</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="245"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="255"/>
      <source>A multiplier factor that affects the size of texts and markers</source>
      <translation>テキストとマーカーのサイズに影響を与える乗算係数</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="248"/>
      <source>Scale multiplier</source>
      <translation>尺度の倍率</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="268"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="278"/>
      <source>If it is checked it will show the unit next to the dimension value</source>
      <translation>これにチェックが入っている場合、寸法の値に続いて単位が表示されます。</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="271"/>
      <source>Show unit</source>
      <translation>単位を表示</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="291"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="301"/>
      <source>Specify a valid length unit like mm, m, in, ft, to force displaying the dimension value in this unit</source>
      <translation>mm、m、in、ftのような有効な長さの単位を指定して、この単位で寸法値を強制的に表示します。</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="294"/>
      <source>Unit override</source>
      <translation>単位の上書き</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="308"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="318"/>
      <source>The number of decimals to show for dimension values</source>
      <translation>寸法の値に表示する小数点以下の桁数</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="311"/>
      <source>Decimals</source>
      <translation>小数点以下の桁数</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="331"/>
      <source>Line and arrows</source>
      <translation>線と矢印</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="337"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="347"/>
      <source>If it is checked it will display the dimension line</source>
      <translation>ここにチェックが入っている場合に、寸法線が表示されます</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="340"/>
      <source>Show lines</source>
      <translation>Show lines</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="363"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="373"/>
      <source>The width of the dimension lines</source>
      <translation>寸法線の幅</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="366"/>
      <source>Line width</source>
      <translation>ライン幅</translation>
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
      <translation>寸法線の矢印やテキストの色</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="389"/>
      <source>Line / text color</source>
      <translation>線/文の字色</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="410"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="432"/>
      <source>The type of arrows or markers to use at the end of dimension lines</source>
      <translation>寸法線の端点で用いる矢印やマーカーの種類</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="413"/>
      <source>Arrow type</source>
      <translation>矢印のタイプ</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="436"/>
      <source>Dot</source>
      <translation>点</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="441"/>
      <source>Circle</source>
      <translation>円</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="446"/>
      <source>Arrow</source>
      <translation>矢印</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="451"/>
      <source>Tick</source>
      <translation>目盛</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="456"/>
      <source>Tick-2</source>
      <translation>目盛り-2</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="464"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="474"/>
      <source>The size of the dimension arrows or markers in system units</source>
      <translation>システム単位における寸法矢印やマーカーのサイズ</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="467"/>
      <source>Arrow size</source>
      <translation>矢印のサイズ</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="484"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="494"/>
      <source>The distance that the dimension line is additionally extended</source>
      <translation>寸法線がさらに延長される距離</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="487"/>
      <source>Dimension overshoot</source>
      <translation>寸法線の超過量</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="504"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="514"/>
      <source>The length of the extension lines</source>
      <translation>寸法補助線の長さ</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="507"/>
      <source>Extension lines</source>
      <translation>補助線</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="524"/>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="534"/>
      <source>The distance that the extension lines are additionally extended beyond the dimension line</source>
      <translation>寸法補助線が寸法線を超えて延長される距離</translation>
    </message>
    <message>
      <location filename="../ui/dialog_AnnotationStyleEditor.ui" line="527"/>
      <source>Extension overshoot</source>
      <translation>寸法補助線の超過量</translation>
    </message>
  </context>
  <context>
    <name>Draft</name>
    <message>
      <location filename="../../importDXF.py" line="129"/>
      <source>Download of dxf libraries failed.
Please install the dxf Library addon manually
from menu Tools -&gt; Addon Manager</source>
      <translation>dxfライブラリーのダウンロードに失敗しました。
メニューのツール→アドオンマネージャーからdxfライブラリーアドオンを手動でインストールしてください。</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="133"/>
      <location filename="../../InitGui.py" line="134"/>
      <location filename="../../InitGui.py" line="135"/>
      <location filename="../../InitGui.py" line="136"/>
      <location filename="../../InitGui.py" line="137"/>
      <source>Draft</source>
      <translation>喫水</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="179"/>
      <location filename="../../InitGui.py" line="180"/>
      <location filename="../../InitGui.py" line="181"/>
      <location filename="../../InitGui.py" line="182"/>
      <source>Import-Export</source>
      <translation>インポート/エクスポート</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="248"/>
      <source>Toggles Grid On/Off</source>
      <translation>グリッドのオン/オフを切り替え</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="265"/>
      <source>Object snapping</source>
      <translation>オブジェクトスナップ</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="297"/>
      <source>Toggles Visual Aid Dimensions On/Off</source>
      <translation>表示補助寸法のオン/オフを切り替え</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="316"/>
      <source>Toggles Ortho On/Off</source>
      <translation>矩形のオン/オフを切り替え</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_draft_statusbar.py" line="334"/>
      <source>Toggles Constrain to Working Plane On/Off</source>
      <translation>作業平面への拘束のオン/オフを切り替え</translation>
    </message>
    <message>
      <location filename="../../draftobjects/bspline.py" line="106"/>
      <source>_BSpline.createGeometry: Closed with same first/last Point. Geometry not updated.</source>
      <translation>_BSpline.createGeometry：最初/最後の点を一致させて曲線を閉じてください。ジオメトリーは更新されていません。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="244"/>
      <location filename="../../draftobjects/pointarray.py" line="306"/>
      <source>Point object doesn't have a discrete point, it cannot be used for an array.</source>
      <translation>点オブジェクトに離散点がありません。整列には使用できません。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="88"/>
      <location filename="../../draftguitools/gui_lineslope.py" line="91"/>
      <source>Slope</source>
      <translation>傾斜</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="91"/>
      <source>Clone</source>
      <translation>クローン</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="49"/>
      <source>You must choose a base object before using this command</source>
      <translation>このコマンドを使用する前にベースオブジェクトを選択する必要があります</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="89"/>
      <source>Delete original objects</source>
      <translation>元のオブジェクトを削除</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="95"/>
      <source>Create chamfer</source>
      <translation>面取りを作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="274"/>
      <source>Save style</source>
      <translation>スタイルを保存</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="275"/>
      <source>Name of this new style:</source>
      <translation>この新しいスタイルの名前:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="282"/>
      <source>Warning</source>
      <translation>警告</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="283"/>
      <source>Name exists. Overwrite?</source>
      <translation>名前が存在します。上書きしますか？</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="318"/>
      <source>Error: json module not found. Unable to save style</source>
      <translation>エラー：json モジュールが見つかりません。スタイルを保存できません。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="329"/>
      <source>Offset direction is not defined. Please move the mouse on either side of the object first to indicate a direction</source>
      <translation>オフセット方向が定義されていません。最初にオブジェクトのどちらか一方にマウスを動かして方向を指定してください。</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="144"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="156"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="157"/>
      <source>True</source>
      <translation>True</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="148"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="160"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="159"/>
      <source>False</source>
      <translation>False</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="151"/>
      <source>Scale</source>
      <translation>尺度</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="152"/>
      <source>X factor</source>
      <translation>X係数</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="153"/>
      <source>Y factor</source>
      <translation>Y係数</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="154"/>
      <source>Z factor</source>
      <translation>Z係数</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="155"/>
      <source>Uniform scaling</source>
      <translation>均一な拡大縮小</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="156"/>
      <source>Working plane orientation</source>
      <translation>作業平面の向き</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="157"/>
      <source>Copy</source>
      <translation>コピー</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="158"/>
      <source>Modify subelements</source>
      <translation>サブ要素を変更</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="159"/>
      <source>Pick from/to points</source>
      <translation>点から選択</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_scale.py" line="160"/>
      <source>Create a clone</source>
      <translation>クローンを作成</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_wpproxy.py" line="102"/>
      <source>Writing camera position</source>
      <translation>カメラ位置を書き込み</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_wpproxy.py" line="119"/>
      <source>Writing objects shown/hidden state</source>
      <translation>オブジェクトの表示/非表示の状態を書き込み</translation>
    </message>
  </context>
  <context>
    <name>DraftCircularArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="26"/>
      <source>Circular array</source>
      <translation>円形整列</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(アイコンのプレースホルダ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="53"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="63"/>
      <source>Distance from one layer of objects to the next layer of objects.</source>
      <translation>オブジェクトのレイヤーから次のオブジェクトのレイヤーまでの距離。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="56"/>
      <source>Radial distance</source>
      <translation>半径距離</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="76"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="87"/>
      <source>Distance from one element in one ring of the array to the next element in the same ring.
It cannot be zero.</source>
      <translation>整列円の要素から同じ円内の次の要素までの距離。
ゼロにはできません。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="80"/>
      <source>Tangential distance</source>
      <translation>接線距離</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="101"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="112"/>
      <source>Number of circular layers or rings to create, including a copy of the original object.
It must be at least 2.</source>
      <translation>元オブジェクトのコピーを含む、作成する円形レイヤーまたは円の数。
2以上でなければなりません。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="105"/>
      <source>Number of circular layers</source>
      <translation>円形レイヤーの数</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="126"/>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="136"/>
      <source>The number of symmetry lines in the circular array.</source>
      <translation>円形整列内の対称線の数。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="129"/>
      <source>Symmetry</source>
      <translation>対称</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="151"/>
      <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
      <translation>回転軸が通過する点の座標。
プロパティ・エディターより軸自身の方向を変更できます。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="155"/>
      <source>Center of rotation</source>
      <translation>回転中心</translation>
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
      <translation>回転中心の座標をリセット</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="228"/>
      <source>Reset point</source>
      <translation>ポイントをリセット</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="240"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>チェックされている場合、互いに接触している整列の結果オブジェクトは結合されます。
この機能は「リンク整列」がオフの場合にのみ動作します。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="244"/>
      <source>Fuse</source>
      <translation>結合</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="251"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>チェックされている場合、結果オブジェクトは通常整列ではなく「リンク整列」になります。
複数のコピーを作成する場合、リンク整列の方が効率的ですが結合することはできなくなります。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_CircularArray.ui" line="255"/>
      <source>Link array</source>
      <translation>リンク整列</translation>
    </message>
  </context>
  <context>
    <name>DraftOrthoArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="26"/>
      <source>Orthogonal array</source>
      <translation>矩形整列</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(アイコンのプレースホルダ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="51"/>
      <source>Number of elements in the array in the specified direction, including a copy of the original object.
The number must be at least 1 in each direction.</source>
      <translation>元オブジェクトのコピーを含む、指定方向の整列要素の数。
各方向の値は、1以上でなければなりません。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="55"/>
      <source>Number of elements</source>
      <translation>要素の数</translation>
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
      <translation>X 方向における要素間の距離。
通常ではXの値のみ必要とし、他に２つの値を与えるとそれぞれの距離に従ってシフトさせます。
負の値を指定すると、それら負の方向にコピーされます。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="124"/>
      <source>X intervals</source>
      <translation>X の間隔</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="197"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="288"/>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="379"/>
      <source>Reset the distances.</source>
      <translation>距離をリセットする</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="200"/>
      <source>Reset X</source>
      <translation>X をリセット</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="210"/>
      <source>Distance between the elements in the Y direction.
Normally, only the Y value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Y 方向における要素間の距離。
通常ではYの値のみ必要とし、他に２つの値を与えるとそれぞれの距離に従ってシフトさせます。
負の値を指定すると、それら負の方向にコピーされます。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="215"/>
      <source>Y intervals</source>
      <translation>Y の間隔</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="291"/>
      <source>Reset Y</source>
      <translation>Y をリセット</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="301"/>
      <source>Distance between the elements in the Z direction.
Normally, only the Z value is necessary; the other two values can give an additional shift in their respective directions.
Negative values will result in copies produced in the negative direction.</source>
      <translation>Z方向における要素間の距離。
通常ではZの値のみ必要とし、他に２つの値を与えるとそれぞれの距離に従ってシフトさせます。
負の値を指定すると、それら負の方向にコピーされます。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="306"/>
      <source>Z intervals</source>
      <translation>Z の間隔</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="382"/>
      <source>Reset Z</source>
      <translation>Zをリセット</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="394"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>チェックされている場合、互いに接触している整列の結果オブジェクトは結合されます。
この機能は「リンク整列」がオフの場合にのみ動作します。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="398"/>
      <source>Fuse</source>
      <translation>結合</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="405"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>チェックされている場合、結果オブジェクトは通常整列ではなく「リンク整列」になります。
複数のコピーを作成する場合、リンク整列の方が効率的ですが結合することはできなくなります。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_OrthoArray.ui" line="409"/>
      <source>Link array</source>
      <translation>リンク整列</translation>
    </message>
  </context>
  <context>
    <name>DraftPolarArrayTaskPanel</name>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="26"/>
      <source>Polar array</source>
      <translation>軸周整列</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="44"/>
      <source>(Placeholder for the icon)</source>
      <translation>(アイコンのプレースホルダ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="53"/>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="65"/>
      <source>Sweeping angle of the polar distribution.
A negative angle produces a polar pattern in the opposite direction.
The maximum absolute value is 360 degrees.</source>
      <translation>軸周配置のスイープ角度。
マイナスの角度では反対方向に軸周パターンが生成されます。
絶対最大値は360度です。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="58"/>
      <source>Polar angle</source>
      <translation>軸周角度</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="86"/>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="97"/>
      <source>Number of elements in the array, including a copy of the original object.
It must be at least 2.</source>
      <translation>元オブジェクトのコピーを含む、整列内の要素の数。
2以上でなければなりません。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="90"/>
      <source>Number of elements</source>
      <translation>要素の数</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="113"/>
      <source>The coordinates of the point through which the axis of rotation passes.
Change the direction of the axis itself in the property editor.</source>
      <translation>回転軸が通過する点の座標。
プロパティ・エディターより軸自身の方向を変更できます。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="117"/>
      <source>Center of rotation</source>
      <translation>回転中心</translation>
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
      <translation>回転中心の座標をリセット</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="190"/>
      <source>Reset point</source>
      <translation>ポイントをリセット</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="202"/>
      <source>If checked, the resulting objects in the array will be fused if they touch each other.
This only works if "Link array" is off.</source>
      <translation>チェックされている場合、互いに接触している整列の結果オブジェクトは結合されます。
この機能は「リンク整列」がオフの場合にのみ動作します。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="206"/>
      <source>Fuse</source>
      <translation>結合</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="213"/>
      <source>If checked, the resulting object will be a "Link array" instead of a regular array.
A Link array is more efficient when creating multiple copies, but it cannot be fused together.</source>
      <translation>チェックされている場合、結果オブジェクトは通常整列ではなく「リンク整列」になります。
複数のコピーを作成する場合、リンク整列の方が効率的ですが結合することはできなくなります。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_PolarArray.ui" line="217"/>
      <source>Link array</source>
      <translation>リンク整列</translation>
    </message>
  </context>
  <context>
    <name>DraftShapeStringGui</name>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="26"/>
      <source>ShapeString</source>
      <translation>シェイプストリング</translation>
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
      <translation>座標を入力するかマウスで点を選択。</translation>
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
      <translation>3D点選択をリセット</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="120"/>
      <source>Reset Point</source>
      <translation>ポイントリセット</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="131"/>
      <source>String</source>
      <translation>文字列</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="138"/>
      <source>Text to be made into ShapeString</source>
      <translation>シェイプストリングにするテキスト</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="149"/>
      <source>Height</source>
      <translation>高さ</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="156"/>
      <source>Height of the result</source>
      <translation>結果の高さ</translation>
    </message>
    <message>
      <location filename="../ui/TaskShapeString.ui" line="176"/>
      <source>Font file</source>
      <translation>フォント ファイル</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddConstruction</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="309"/>
      <source>Add to Construction group</source>
      <translation>構築グループに追加</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="312"/>
      <source>Adds the selected objects to the construction group,
and changes their appearance to the construction style.
It creates a construction group if it doesn't exist.</source>
      <translation>選択したオブジェクトを構築グループに追加し、
その外観を構築スタイルに変更します。
存在しない場合は構築グループを作成します。</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddNamedGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="361"/>
      <source>Add a new named group</source>
      <translation type="unfinished">Add a new named group</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="365"/>
      <source>Add a new group with a given name.</source>
      <translation>指定した名前で新しいグループを追加</translation>
    </message>
  </context>
  <context>
    <name>Draft_AddPoint</name>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="57"/>
      <source>Add point</source>
      <translation>点を追加</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="60"/>
      <source>Adds a point to an existing Wire or B-spline.</source>
      <translation>既存のワイヤーまたはB-スプラインに点を追加。</translation>
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
      <translation>選択したオブジェクトを既存のグループに移動するか、グループから削除します。
このツールを使用するにはまずグループを作成してください。</translation>
    </message>
  </context>
  <context>
    <name>Draft_AnnotationStyleEditor</name>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="78"/>
      <source>Annotation styles...</source>
      <translation>注釈スタイル...</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="82"/>
      <source>Manage or create annotation styles</source>
      <translation>注釈スタイルの管理と作成</translation>
    </message>
  </context>
  <context>
    <name>Draft_ApplyStyle</name>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="47"/>
      <source>Apply current style</source>
      <translation>現在のスタイルを適用</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="50"/>
      <source>Applies the current style defined in the toolbar (line width and colors) to the selected objects and groups.</source>
      <translation>ツール バーで定義した現在のスタイル（線の幅と色）を、選択したオブジェクトとグループに適用</translation>
    </message>
  </context>
  <context>
    <name>Draft_Arc</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="66"/>
      <source>Arc</source>
      <translation>円弧</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="69"/>
      <source>Creates a circular arc by a center point and a radius.
CTRL to snap, SHIFT to constrain.</source>
      <translation>中心点と半径を指定して円弧を作成します。 
CTRLでスナップ、SHIFTで拘束。</translation>
    </message>
  </context>
  <context>
    <name>Draft_ArcTools</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="606"/>
      <source>Arc tools</source>
      <translation>円弧ツール</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="609"/>
      <source>Create various types of circular arcs.</source>
      <translation>様々な種類の円弧を作成</translation>
    </message>
  </context>
  <context>
    <name>Draft_Arc_3Points</name>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="487"/>
      <source>Arc by 3 points</source>
      <translation>3点から決まる円弧</translation>
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
      <translation>配列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="71"/>
      <source>Creates an array from a selected object.
By default, it is a 2x2 orthogonal array.
Once the array is created its type can be changed
to polar or circular, and its properties can be modified.</source>
      <translation>選択したオブジェクトから整列を作成します。
デフォルトでは2×2の矩形整列です。
整列を作成した後、タイプを軸周または円形に変更したり、
プロパティーを変更したりすることができます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_ArrayTools</name>
    <message>
      <location filename="../../draftguitools/gui_arrays.py" line="65"/>
      <source>Array tools</source>
      <translation>整列ツール</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arrays.py" line="68"/>
      <source>Create various types of arrays, including rectangular, polar, circular, path, and point</source>
      <translation>格子、軸周、円形、経路、点など、さまざまな種類の整列を作成</translation>
    </message>
  </context>
  <context>
    <name>Draft_AutoGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="208"/>
      <source>Autogroup</source>
      <translation>オートグループ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="211"/>
      <source>Select a group to add all Draft and Arch objects to.</source>
      <translation>全てのDraft、Archオブジェクトを追加するグループを選択</translation>
    </message>
  </context>
  <context>
    <name>Draft_BSpline</name>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="60"/>
      <source>B-spline</source>
      <translation>B-スプライン</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="63"/>
      <source>Creates a multiple-point B-spline. CTRL to snap, SHIFT to constrain.</source>
      <translation>複数点からなるB-スプラインを作成します。CTRLでスナップ、SHIFTで拘束。</translation>
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
      <translation>円</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_circles.py" line="84"/>
      <source>Creates a circle (full circular arc).
CTRL to snap, ALT to select tangent objects.</source>
      <translation>円（完全円形弧）を作成します。CTRLでスナップ、ALTで接するオブジェクトを選択.</translation>
    </message>
  </context>
  <context>
    <name>Draft_CircularArray</name>
    <message>
      <location filename="../../draftguitools/gui_circulararray.py" line="65"/>
      <source>Circular array</source>
      <translation>円形整列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_circulararray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in a radial pattern
creating various circular layers.

The array can be turned into an orthogonal or a polar array by changing its type.</source>
      <translation>選択したオブジェクトのコピーを作成し、さまざまな円形レイヤーを作る放射パターンにそれらコピーを配置します。

タイプを変更すると整列を矩形整列、軸周整列へ変えることができます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Clone</name>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="70"/>
      <source>Clone</source>
      <translation>クローン</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="73"/>
      <source>Creates a clone of the selected objects.
The resulting clone can be scaled in each of its three directions.</source>
      <translation>選択したオブジェクトのクローンを作成します。
結果のクローンは、各3方向に拡大縮小できます。</translation>
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
      <translation>点を削除</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_line_add_delete.py" line="92"/>
      <source>Removes a point from an existing Wire or B-spline.</source>
      <translation>既存のワイヤーまたはB-スプラインから点を削除</translation>
    </message>
  </context>
  <context>
    <name>Draft_Dimension</name>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="84"/>
      <source>Dimension</source>
      <translation>寸法</translation>
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
      <translation>寸法を作成します。

- 3点をピックするとシンプルな長さ寸法を作成します。
- 直線を選択すると直線にリンクした長さ寸法を作成します。
- 円弧、円を選択すると円弧にリンクした半径、直径寸法を作成します。
- 2直線を選択すると直線間の角度寸法を作成します。

このコマンドを起動する前に1つの直線、または1つの円弧を選択すると対応するリンクされた寸法を作成します。
またこのコマンドを起動する前に「App::MeasureDistance」オブジェクトを選択するとそれを「Draft Dimension」オブジェクトに変換します。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Downgrade</name>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="58"/>
      <source>Downgrade</source>
      <translation>ダウングレード</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="61"/>
      <source>Downgrades the selected objects into simpler shapes.
The result of the operation depends on the types of objects, which may be able to be downgraded several times in a row.
For example, it explodes the selected polylines into simpler faces, wires, and then edges. It can also subtract faces.</source>
      <translation>選択したオブジェクトをより単純なシェイプへダウングレード。
操作結果はオブジェクトの種類によって代わり、続けて複数回ダウングレードを行えます。
例えば、選択したポリラインを単純な面、ワイヤー、さらにはエッジへ分解できます。また面同士の減算も可能です。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Draft2Sketch</name>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="57"/>
      <source>Draft to Sketch</source>
      <translation>ドラフトからスケッチへ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="60"/>
      <source>Convert bidirectionally between Draft objects and Sketches.
Many Draft objects will be converted into a single non-constrained Sketch.
However, a single sketch with disconnected traces will be converted into several individual Draft objects.</source>
      <translation>ドラフト・オブジェクトとスケッチを相互に変換。
ドラフト・オブジェクトの多くは1つの拘束されていないスケッチに変換されます。
一方で繋がっていない1つのスケッチは複数の別れたドラフト・オブジェクトに変換されます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Drawing</name>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="71"/>
      <source>Drawing</source>
      <translation>図面</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="74"/>
      <source>Creates a 2D projection on a Drawing Workbench page from the selected objects.
This command is OBSOLETE since the Drawing Workbench became obsolete in 0.17.
Use TechDraw Workbench instead for generating technical drawings.</source>
      <translation>Drawingワークベンチのページ上に、選択したオブジェクトから2D投影を作成します。
Drawingワークベンチは 0.17 で廃止されたのでこのコマンドは廃止されました。
製図図面を作成するには代わりにTechDrawワークベンチを使用してください。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Edit</name>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="275"/>
      <source>Edit</source>
      <translation>編集</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="284"/>
      <source>Edits the active object.
Press E or ALT+LeftClick to display context menu
on supported nodes and on supported objects.</source>
      <translation>アクティブなオブジェクトを編集します。
E キーまたは Alt+左クリックを押すと、サポートされている節点、オブジェクト上でコンテキストメニューを表示します。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Ellipse</name>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="59"/>
      <source>Ellipse</source>
      <translation>楕円</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="62"/>
      <source>Creates an ellipse. CTRL to snap.</source>
      <translation>楕円を作成します。CTRLでスナップ。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Facebinder</name>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="60"/>
      <source>Facebinder</source>
      <translation>フェイスバインダー</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="62"/>
      <source>Creates a facebinder object from selected faces.</source>
      <translation>選択した面からフェイスバインダー・オブジェクトを作成</translation>
    </message>
  </context>
  <context>
    <name>Draft_Fillet</name>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="64"/>
      <source>Fillet</source>
      <translation>フィレット</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="67"/>
      <source>Creates a fillet between two selected wires or edges.</source>
      <translation>2 つの選択したワイヤまたはエッジの間にフィレットを作成</translation>
    </message>
  </context>
  <context>
    <name>Draft_FlipDimension</name>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="62"/>
      <source>Flip dimension</source>
      <translation>寸法を反転</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="66"/>
      <source>Flip the normal direction of the selected dimensions (linear, radial, angular).
If other objects are selected they are ignored.</source>
      <translation>選択した寸法（長さ、半径、角度）の法線方向を反転させます。
他のオブジェクトが選択されている場合は無視されます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Hatch</name>
    <message>
      <location filename="../../draftguitools/gui_hatch.py" line="38"/>
      <source>Hatch</source>
      <translation>ハッチング</translation>
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
      <translation>修復</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="61"/>
      <source>Heal faulty Draft objects saved with an earlier version of the program.
If an object is selected it will try to heal that object in particular,
otherwise it will try to heal all objects in the active document.</source>
      <translation>以前のバージョンのプログラムで保存された不完全なDraftオブジェクトを修復。
オブジェクトが選択されている場合、そのオブジェクトだけ修復を試みます。
それ以外の場合はアクティブなドキュメント内の全てのオブジェクトの修復を試みます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Join</name>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="66"/>
      <source>Join</source>
      <translation>接合</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="69"/>
      <source>Joins the selected lines or polylines into a single object.
The lines must share a common point at the start or at the end for the operation to succeed.</source>
      <translation>選択した線またはポリラインを1つのオブジェクトに結合します。
操作を成功させるには、線が始点または終点で点を共有している必要があります。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Label</name>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="64"/>
      <source>Label</source>
      <translation>ラベル</translation>
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
      <translation>ラベルを作成し、オプションで選択したオブジェクトやサブ要素に設定します。

まず頂点やエッジ、オブジェクトの面を選択し、このコマンドを実行します。
その後、引き出し線とテキストラベルの位置を設定してください。
ラベルにはオブジェクトや選択したサブ要素などの情報を表示することができます。

複数のオブジェクトやサブ要素が選択されている場合、ラベルでの情報表示に使われるのそれぞれ最初のものだけです。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Layer</name>
    <message>
      <location filename="../../draftguitools/gui_layers.py" line="52"/>
      <source>Layer</source>
      <translation>レイヤー</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_layers.py" line="55"/>
      <source>Adds a layer to the document.
Objects added to this layer can share the same visual properties such as line color, line width, and shape color.</source>
      <translation>レイヤーをドキュメントに追加します。
このレイヤーに追加されたオブジェクトは、線の色、線の幅、形状の色など、同じ表示プロパティーを共有できます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Line</name>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="63"/>
      <source>Line</source>
      <translation>直線</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="66"/>
      <source>Creates a 2-point line. CTRL to snap, SHIFT to constrain.</source>
      <translation>2点を指定して線を作成します。CTRLでスナップ、SHIFTで拘束。</translation>
    </message>
  </context>
  <context>
    <name>Draft_LinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="119"/>
      <source>LinkArray</source>
      <translation>リンク整列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="122"/>
      <source>Like the Array tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>整列ツールと同様ですが「リンク整列」を作成します。
「リンク整列」では多数のコピーをより効率的に制御できますが、「Fuse」オプションは使用できません。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Mirror</name>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="63"/>
      <source>Mirror</source>
      <translation>鏡像コピー</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="66"/>
      <source>Mirrors the selected objects along a line defined by two points.</source>
      <translation>2点で定義された線に沿って選択したオブジェクトを鏡像コピー。</translation>
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
If the "copy" option is active, it will create displaced copies.
CTRL to snap, SHIFT to constrain.</source>
      <translation>選択したオブジェクトを、ある基準点から別の点へ移動します。
「コピー」オプションが有効な場合、移動したコピーを作成します。
CTRLでスナップ、SHIFTで拘束。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Offset</name>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="63"/>
      <source>Offset</source>
      <translation>オフセット</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="66"/>
      <source>Offsets of the selected object.
It can also create an offset copy of the original object.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
      <translation>選択したオブジェクトのオフセット。
元のオブジェクトのオフセットコピーを作成することもできます。 
CTRLでスナップ、SHIFTで拘束します。ALTキーを押しながらクリックするごとにコピーを作成します。</translation>
    </message>
  </context>
  <context>
    <name>Draft_OrthoArray</name>
    <message>
      <location filename="../../draftguitools/gui_orthoarray.py" line="65"/>
      <source>Array</source>
      <translation>配列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_orthoarray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in an orthogonal pattern,
meaning the copies follow the specified direction in the X, Y, Z axes.

The array can be turned into a polar or a circular array by changing its type.</source>
      <translation>選択したオブジェクトのコピーを作成し、矩形パターンそれらを配置することで
X、Y、Zの軸の指定方向にコピーを並べます。

タイプを変更すると整列を軸周整列、円形整列へ変えることができます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathArray</name>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="73"/>
      <source>Path array</source>
      <translation>パス整列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="76"/>
      <source>Creates copies of the selected object along a selected path.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
      <translation>選択したパスに沿って選択したオブジェクトのコピーを作成します。
まずオブジェクトを選択し、次にパスを選択してください。
パスにはポリライン、B-スプライン、ベジェ曲線、または別のオブジェクトのエッジを使用できます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="163"/>
      <source>Path Link array</source>
      <translation>パスリンク整列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="166"/>
      <source>Like the PathArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>パス整列ツールと同様ですが「リンク整列」を作成します。
「リンク整列」では多数のコピーをより効率的に制御できますが、「Fuse」オプションは使用できません。</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathTwistedArray</name>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="66"/>
      <source>Path twisted array</source>
      <translation>パスツイスト整列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="69"/>
      <source>Creates copies of the selected object along a selected path, and twists the copies.
First select the object, and then select the path.
The path can be a polyline, B-spline, Bezier curve, or even edges from other objects.</source>
      <translation>選択したパスに沿って選択したオブジェクトのコピーを作成し、さらにコピーを回転させます。
まずオブジェクトを選択し、次にパスを選択してください。
パスにはポリライン、B-スプライン、ベジェ曲線、または別のオブジェクトのエッジを使用できます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_PathTwistedLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="121"/>
      <source>Path twisted Link array</source>
      <translation>パスツイストリンク整列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="124"/>
      <source>Like the PathTwistedArray tool, but creates a 'Link array' instead.
A 'Link array' is more efficient when handling many copies but the 'Fuse' option cannot be used.</source>
      <translation>パスツイスト整列ツールと同様ですが「リンク整列」を作成します。
「リンク整列」では多数のコピーをより効率的に制御できますが、「Fuse」オプションは使用できません。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Point</name>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="62"/>
      <source>Point</source>
      <translation>点</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="65"/>
      <source>Creates a point object. Click anywhere on the 3D view.</source>
      <translation>点オブジェクトを作成します。3Dビューの任意の場所をクリックしてください。</translation>
    </message>
  </context>
  <context>
    <name>Draft_PointArray</name>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="76"/>
      <source>Point array</source>
      <translation>点整列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="79"/>
      <source>Creates copies of the selected object, and places the copies at the position of various points.

The points need to be grouped under a compound of points before using this tool.
To create this compound, select various points and then use the Part Compound tool,
or use the Draft Upgrade tool to create a 'Block', or create a Sketch and add simple points to it.

Select the base object, and then select the compound or the sketch to create the point array.</source>
      <translation>選択したオブジェクトのコピーを作成し、さまざまな点の位置にそれを配置します。

このツールを使用する前に点コンパウンドの下に点をまとめておく必要があります。
このコンパウンドを作成するためには、点群を選択した後でパート・コンパウンドツールを使用するか、
またはドラフト・アップグレードツールで「ブロック」を作成するか、またはスケッチを作成してそこに点を追加してください。

ベース・オブジェクトを選択した後、コンパウンドまたはスケッチを選択して点整列を作成します。</translation>
    </message>
  </context>
  <context>
    <name>Draft_PointLinkArray</name>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="141"/>
      <source>PointLinkArray</source>
      <translation>点リンク整列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="144"/>
      <source>Like the PointArray tool, but creates a 'Point link array' instead.
A 'Point link array' is more efficient when handling many copies.</source>
      <translation>点整列ツールと同様ですが「点リンク整列」を作成します。
「点リンク整列」では多数のコピーをより効率的に制御できます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_PolarArray</name>
    <message>
      <location filename="../../draftguitools/gui_polararray.py" line="65"/>
      <source>Polar array</source>
      <translation>軸周整列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polararray.py" line="68"/>
      <source>Creates copies of the selected object, and places the copies in a polar pattern
defined by a center of rotation and its angle.

The array can be turned into an orthogonal or a circular array by changing its type.</source>
      <translation>選択したオブジェクトのコピーを作成し、回転中心と角度で定義された軸周パターンにそれらを配置します。

タイプを変更すると整列を格子整列、円形整列へ変えることができます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Polygon</name>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="57"/>
      <source>Polygon</source>
      <translation>多角形</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="60"/>
      <source>Creates a regular polygon (triangle, square, pentagon, ...), by defining the number of sides and the circumscribed radius.
CTRL to snap, SHIFT to constrain</source>
      <translation>辺の数と外接半径を定義して、正多角形（三角形、正方形、五角形...）を作成します。
CTRLでスナップ、SHIFTで拘束。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Rectangle</name>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="54"/>
      <source>Rectangle</source>
      <translation>四角形</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="57"/>
      <source>Creates a 2-point rectangle. CTRL to snap.</source>
      <translation>2点を指定して四角形を作成します。CTRLでスナップ。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Rotate</name>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="62"/>
      <source>Rotate</source>
      <translation>回転</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="65"/>
      <source>Rotates the selected objects. Choose the center of rotation, then the initial angle, and then the final angle.
If the "copy" option is active, it will create rotated copies.
CTRL to snap, SHIFT to constrain. Hold ALT and click to create a copy with each click.</source>
      <translation>選択したオブジェクトを回転させます。回転の中心、初期角度、最終角度を選択してください。
「コピー」オプションが有効な場合、回転したコピーが作成されます。 
CTRLでスナップ、SHIFTで拘束します。ALTキーを押しながらクリックするごとにコピーを作成します。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Scale</name>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="71"/>
      <source>Scale</source>
      <translation>尺度</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="74"/>
      <source>Scales the selected objects from a base point.
CTRL to snap, SHIFT to constrain, ALT to copy.</source>
      <translation>選択したオブジェクトを基準点を中心に拡大します。
CTRLでスナップ、SHIFTで拘束、ALTでコピー。</translation>
    </message>
  </context>
  <context>
    <name>Draft_SelectGroup</name>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="164"/>
      <source>Select group</source>
      <translation>グループを選択</translation>
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
      <translation>ドラフトオブジェクトをスケッチする作業平面を作成する、ソリッドボディーの面を選択します。
3 つの頂点または作業平面プロキシを選択することもできます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_SetStyle</name>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="55"/>
      <source>Set style</source>
      <translation>スタイルを設定</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_setstyle.py" line="57"/>
      <source>Sets default styles</source>
      <translation>デフォルトのスタイルを設定</translation>
    </message>
  </context>
  <context>
    <name>Draft_Shape2DView</name>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="59"/>
      <source>Shape 2D view</source>
      <translation>2Dビュー形状</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="62"/>
      <source>Creates a 2D projection of the selected objects on the XY plane.
The initial projection direction is the negative of the current active view direction.
You can select individual faces to project, or the entire solid, and also include hidden lines.
These projections can be used to create technical drawings with the TechDraw Workbench.</source>
      <translation>選択したオブジェクトのXY平面上への2D投影を作成。
初期の投影方向は現在のアクティブビュー方向の逆方向です。
個々の面を投影するか、ソリッド全体を投影するか、また隠線を含めるかどうかを選択できます。
こうした投影はTechDrawワークベンチでの製図を作成するために使用できます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_ShapeString</name>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="66"/>
      <source>Shape from text</source>
      <translation>テキストからシェイプへ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="68"/>
      <source>Creates a shape from a text string by choosing a specific font and a placement.
The closed shapes can be used for extrusions and boolean operations.</source>
      <translation>特定のフォントと配置を選択して、テキスト文字列からシェイプを作成します。
閉じたシェイプは、押し出しやブール演算に使用できます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_ShowSnapBar</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="589"/>
      <source>Show snap toolbar</source>
      <translation>スナップ用ツールバーを表示</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="592"/>
      <source>Show the snap toolbar if it is hidden.</source>
      <translation>非表示の場合はスナップ用ツールバーを表示します。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Slope</name>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="70"/>
      <source>Set slope</source>
      <translation>傾斜を設定</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="73"/>
      <source>Sets the slope of the selected line by changing the value of the Z value of one of its points.
If a polyline is selected, it will apply the slope transformation to each of its segments.

The slope will always change the Z value, therefore this command only works well for
straight Draft lines that are drawn in the XY plane. Selected objects that aren't single lines will be ignored.</source>
      <translation>点のZ値を変更することで選択した線に傾斜を設定します。
ポリラインを選択した場合は各線分に対して傾斜変形を適用します。

傾斜では常にZ値を変更するので、このコマンドはXY平面に描かれた直線のドラフト線のみで正しく動作します。線以外の選択オブジェクトは無視されます。</translation>
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
      <translation>30度と45度の角度の倍数に位置する円弧上の点にスナップを設定</translation>
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
      <translation>円弧の中心にスナップを設定</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Dimensions</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="526"/>
      <source>Show dimensions</source>
      <translation>寸法を表示</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="529"/>
      <source>Show temporary linear dimensions when editing an object and using other snapping methods.</source>
      <translation>オブジェクトを編集する時や他のスナップ方法を使用する時に、一時的な長さ寸法を表示</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Endpoint</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="313"/>
      <source>Endpoint</source>
      <translation>端点</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="316"/>
      <source>Set snapping to endpoints of an edge.</source>
      <translation>エッジの端点にスナップを設定</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Extension</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="404"/>
      <source>Extension</source>
      <translation>拡張</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="407"/>
      <source>Set snapping to the extension of an edge.</source>
      <translation>エッジの延長にスナップを設定</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Grid</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="223"/>
      <source>Grid</source>
      <translation>グリッド</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="226"/>
      <source>Set snapping to the intersection of grid lines.</source>
      <translation>グリッド線の交点にスナップを設定</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Intersection</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="253"/>
      <source>Intersection</source>
      <translation>共通集合</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="256"/>
      <source>Set snapping to the intersection of edges.</source>
      <translation>エッジの交点にスナップを設定</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Lock</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="133"/>
      <source>Main snapping toggle On/Off</source>
      <translation>メインスナップのオン/オフを切り替え</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="136"/>
      <source>Activates or deactivates all snap methods at once.</source>
      <translation>全てのスナップ方法を同時に有効または無効にします。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Midpoint</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="163"/>
      <source>Midpoint</source>
      <translation>中点</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="166"/>
      <source>Set snapping to the midpoint of an edge.</source>
      <translation>エッジの中点にスナップを設定</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Near</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="434"/>
      <source>Nearest</source>
      <translation>最近接</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="437"/>
      <source>Set snapping to the nearest point of an edge.</source>
      <translation>エッジの最近接点にスナップを設定</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Ortho</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="465"/>
      <source>Orthogonal</source>
      <translation>矩形</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="468"/>
      <source>Set snapping to a direction that is a multiple of 45 degrees from a point.</source>
      <translation>点から45度の倍数の方向にスナップを設定</translation>
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
      <translation>エッジに平行な方向にスナップを設定</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Perpendicular</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="193"/>
      <source>Perpendicular</source>
      <translation>直交する|鉛直な</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="196"/>
      <source>Set snapping to a direction that is perpendicular to an edge.</source>
      <translation>エッジに垂直な方向にスナップを設定</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_Special</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="495"/>
      <source>Special</source>
      <translation>特殊</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="498"/>
      <source>Set snapping to the special points defined inside an object.</source>
      <translation>オブジェクト内で定義されている特殊点にスナップを設定</translation>
    </message>
  </context>
  <context>
    <name>Draft_Snap_WorkingPlane</name>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="559"/>
      <source>Working plane</source>
      <translation>作業平面</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="562"/>
      <source>Restricts snapping to a point in the current working plane.
If you select a point outside the working plane, for example, by using other snapping methods,
it will snap to that point's projection in the current working plane.</source>
      <translation>現在の作業平面の点にスナップを制限します。
例えば他のスナップ方法を使用して作業平面外の点を選択した場合、その点を現在の作業平面内に投影した結果にスナップします。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Split</name>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="56"/>
      <source>Split</source>
      <translation>スプリット</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="59"/>
      <source>Splits the selected line or polyline into two independent lines
or polylines by clicking anywhere along the original object.
It works best when choosing a point on a straight segment and not a corner vertex.</source>
      <translation>選択した直線またはポリラインを、元のオブジェクトに沿った任意の位置をクリックすることで2つの独立した直線、またはポリラインに分割します。
曲がり部の頂点ではなく直線線分上の点を選択した時に最良の結果が得られます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Stretch</name>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="64"/>
      <source>Stretch</source>
      <translation>伸縮</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="67"/>
      <source>Stretches the selected objects.
Select an object, then draw a rectangle to pick the vertices that will be stretched,
then draw a line to specify the distance and direction of stretching.</source>
      <translation>選択したオブジェクトを伸縮。
オブジェクトを選択してから、長方形を描いて伸縮させる頂点を選択してください。
その後、伸縮の距離と方向を指定する直線を描いてください。</translation>
    </message>
  </context>
  <context>
    <name>Draft_SubelementHighlight</name>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="61"/>
      <source>Subelement highlight</source>
      <translation>サブ要素のハイライト</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="64"/>
      <source>Highlight the subelements of the selected objects, so that they can then be edited with the move, rotate, and scale tools.</source>
      <translation>選択したオブジェクトのサブ要素をハイライト表示して、移動、回転、拡大縮小ツールで編集できるようにします。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Text</name>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="60"/>
      <source>Text</source>
      <translation>テキスト</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="63"/>
      <source>Creates a multi-line annotation. CTRL to snap.</source>
      <translation>複数行の注釈を作成します。CTRLでスナップ。</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleConstructionMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="99"/>
      <source>Toggle construction mode</source>
      <translation>構築モードの切り替え</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="102"/>
      <source>Toggles the Construction mode.
When this is active, the following objects created will be included in the construction group, and will be drawn with the specified color and properties.</source>
      <translation>構築モードの切り替え。
有効な場合、以降の作成オブジェクトが構築グループに含まれるようになり、指定した色とプロパティで描画されます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleContinueMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="131"/>
      <source>Toggle continue mode</source>
      <translation>継続モードを切り替え</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="133"/>
      <source>Toggles the Continue mode.
When this is active, any drawing tool that is terminated will automatically start again.
This can be used to draw several objects one after the other in succession.</source>
      <translation>継続モードを切り替え。
有効な場合には完了した全ての描画ツールが自動的に再度開始されます。
複数のオブジェクトを連続的に描画するために使用できます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleDisplayMode</name>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="166"/>
      <source>Toggle normal/wireframe display</source>
      <translation>ノーマル表示/ワイヤーフレーム表示を切り替え</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="168"/>
      <source>Switches the display mode of selected objects from flatlines to wireframe and back.
This is helpful to quickly visualize objects that are hidden by other objects.
This is intended to be used with closed shapes and solids, and doesn't affect open wires.</source>
      <translation>選択したオブジェクトの表示モードをフラットラインからワイヤーフレームへ相互に切り替え。
他のオブジェクトに隠れたオブジェクトをすばやく表示するのに便利です。
閉じたシェイプやソリッドに対して使用するためのもので、開いたワイヤーには効果を持ちません。</translation>
    </message>
  </context>
  <context>
    <name>Draft_ToggleGrid</name>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="58"/>
      <source>Toggle grid</source>
      <translation>グリッドを切り替え</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="59"/>
      <source>Toggles the Draft grid on and off.</source>
      <translation>ドラフトグリッドのオン/オフを切り替えます。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Trimex</name>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="76"/>
      <source>Trimex</source>
      <translation>トリメックス</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="82"/>
      <source>Trims or extends the selected object, or extrudes single faces.
CTRL snaps, SHIFT constrains to current segment or to normal, ALT inverts.</source>
      <translation>選択したオブジェクトをトリムまたは延長するか、もしくは単一の面を押し出します。CTRLでスナップ、SHIFTで現在のセグメントまたは垂直方向に拘束、ALTで反転。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Upgrade</name>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="58"/>
      <source>Upgrade</source>
      <translation>アップグレード</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="61"/>
      <source>Upgrades the selected objects into more complex shapes.
The result of the operation depends on the types of objects, which may be able to be upgraded several times in a row.
For example, it can join the selected objects into one, convert simple edges into parametric polylines,
convert closed edges into filled faces and parametric polygons, and merge faces into a single face.</source>
      <translation>選択したオブジェクトをより複雑なシェイプへアップグレード。
操作結果はオブジェクトの種類によって代わり、続けて複数回アップグレードを行えます。
例えば、選択したオブジェクトを1つに結合したり、単純なエッジをパラメトリックなポリラインに変換したり、閉じたエッジを面やパラメトリックなポリゴンへ変換したり、複数の面を1つの面に統合したりすることが可能です。</translation>
    </message>
  </context>
  <context>
    <name>Draft_Wire</name>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="306"/>
      <source>Polyline</source>
      <translation>ポリライン</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="309"/>
      <source>Creates a multiple-points line (polyline). CTRL to snap, SHIFT to constrain.</source>
      <translation>複数点からなる線（ポリライン）を作成します。CTRLでスナップ、SHIFTで拘束。</translation>
    </message>
  </context>
  <context>
    <name>Draft_WireToBSpline</name>
    <message>
      <location filename="../../draftguitools/gui_wire2spline.py" line="64"/>
      <source>Wire to B-spline</source>
      <translation>ワイヤーからB-スプラインへ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_wire2spline.py" line="67"/>
      <source>Converts a selected polyline to a B-spline, or a B-spline to a polyline.</source>
      <translation>選択したポリラインをB-スプラインに、またはB-スプラインをポリラインに変換します。</translation>
    </message>
  </context>
  <context>
    <name>Draft_WorkingPlaneProxy</name>
    <message>
      <location filename="../../draftguitools/gui_planeproxy.py" line="50"/>
      <source>Create working plane proxy</source>
      <translation>作業平面プロキシを作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_planeproxy.py" line="52"/>
      <source>Creates a proxy object from the current working plane.
Once the object is created double click it in the tree view to restore the camera position and objects' visibilities.
Then you can use it to save a different camera position and objects' states any time you need.</source>
      <translation>現在の作業平面からプロキシ・オブジェクトを作成します。
オブジェクトを作成した後、ツリービュー上でダブルクリックするとカメラ位置とオブジェクトの表示状態が復元されます。
これによって異なったカメラ位置とオブジェクトの状態を必要な時にいつでも保存することができます。</translation>
    </message>
  </context>
  <context>
    <name>Form</name>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="14"/>
      <source>Working plane setup</source>
      <translation>作業平面のセットアップ</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="20"/>
      <source>Select a face or working plane proxy or 3 vertices.
Or choose one of the options below</source>
      <translation>面、作業平面プロキシー、または3頂点を選択するか、以下のオプションの1つを選んでください。</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="31"/>
      <source>Sets the working plane to the XY plane (ground plane)</source>
      <translation>作業平面を XY 平面（グラウンド）に設定</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="34"/>
      <source>Top (XY)</source>
      <translation>上面 (XY)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="41"/>
      <source>Sets the working plane to the XZ plane (front plane)</source>
      <translation>作業平面を XZ 平面（フロント）に設定</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="44"/>
      <source>Front (XZ)</source>
      <translation>前面 (XZ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="51"/>
      <source>Sets the working plane to the YZ plane (side plane)</source>
      <translation>作業平面を YZ 平面（サイド）に設定</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="54"/>
      <source>Side (YZ)</source>
      <translation>側面 (YZ)</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="61"/>
      <source>Sets the working plane facing the current view</source>
      <translation>作業平面を現在の視線に相対するよう設定</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="64"/>
      <source>Align to view</source>
      <translation>ビュー方向へ調整</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="71"/>
      <source>The working plane will align to the current
view each time a command is started</source>
      <translation>コマンドを開始される度に、作業平面は
現在のビューで整列します</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="78"/>
      <source>Automatic</source>
      <translation>自動</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="87"/>
      <source>Offset</source>
      <translation>オフセット</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="94"/>
      <source>An optional offset to give to the working plane
above its base position. Use this together with one
of the buttons above</source>
      <translation>基本位置の上に作業平面に与える任意のオフセット。
上方にあるボタンの1つと一緒にこれを使用します</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="106"/>
      <location filename="../ui/TaskSelectPlane.ui" line="118"/>
      <source>If this is selected, the working plane will be
centered on the current view when pressing one
of the buttons above</source>
      <translation>これが選択されている場合、上のボタンから何れかを押したときに、作業平面が現在のビューの中央に配置されます</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="111"/>
      <source>Center plane on view</source>
      <translation>ビューの中心面</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="135"/>
      <source>Or select a single vertex to move the current
working plane without changing its orientation.
Then, press the button below</source>
      <translation>または頂点を1つ選択して、現在の
作業平面を方向を変えずに移動。
次に下のボタンを押してください。</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="147"/>
      <source>Moves the working plane without changing its
orientation. If no point is selected, the plane
will be moved to the center of the view</source>
      <translation>方向を変えずに作業平面を移動。
点が選択されていない場合には平面はビューの中心へ移動します。</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="152"/>
      <source>Move working plane</source>
      <translation>作業平面を移動</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="161"/>
      <location filename="../ui/TaskSelectPlane.ui" line="171"/>
      <source>The spacing between the smaller grid lines</source>
      <translation>グリッドの細線の間隔</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="164"/>
      <source>Grid spacing</source>
      <translation>グリッド間隔</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="181"/>
      <location filename="../ui/TaskSelectPlane.ui" line="191"/>
      <source>The number of squares between each main line of the grid</source>
      <translation>グリッドの各主線の間の四角形の数</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="184"/>
      <source>Main line every</source>
      <translation>主線ごと</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="198"/>
      <source>Grid extension</source>
      <translation>グリッド拡張</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="205"/>
      <source> lines</source>
      <translation> 線</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="218"/>
      <location filename="../ui/TaskSelectPlane.ui" line="230"/>
      <source>The distance at which a point can be snapped to
when approaching the mouse. You can also change this
value by using the [ and ] keys while drawing</source>
      <translation>マウスが近づいた時にスナップされる点までの距離。
製図中に [ キー、] キーを使用して変更することもできます。</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="223"/>
      <source>Snapping radius</source>
      <translation>スナップ半径</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="241"/>
      <source>Centers the view on the current working plane</source>
      <translation>現在の作業平面に表示を合わせる</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="244"/>
      <source>Center view</source>
      <translation>中央に表示</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="251"/>
      <source>Resets the working plane to its previous position</source>
      <translation>作業平面を以前の位置にリセット</translation>
    </message>
    <message>
      <location filename="../ui/TaskSelectPlane.ui" line="254"/>
      <source>Previous</source>
      <translation>戻る</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="14"/>
      <source>Style settings</source>
      <translation>スタイル設定</translation>
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
      <translation>線と面</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="66"/>
      <source>Line color</source>
      <translation>線の色</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="73"/>
      <source>The color of lines</source>
      <translation>線の色</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="80"/>
      <source>Line width</source>
      <translation>ライン幅</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="87"/>
      <source> px</source>
      <translation>px</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="94"/>
      <source>Draw style</source>
      <translation>描画スタイル</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="101"/>
      <source>The line style</source>
      <translation>線のスタイル</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="105"/>
      <source>Solid</source>
      <translation>ソリッド</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="110"/>
      <source>Dashed</source>
      <translation>破線</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="115"/>
      <source>Dotted</source>
      <translation>点線</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="120"/>
      <source>DashDot</source>
      <translation>一点鎖線</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="128"/>
      <source>Display mode</source>
      <translation>表示モード</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="135"/>
      <source>The display mode for faces</source>
      <translation>面の表示モード</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="139"/>
      <source>Flat Lines</source>
      <translation>平坦な線</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="144"/>
      <source>Wireframe</source>
      <translation>ワイヤ フレーム</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="149"/>
      <source>Shaded</source>
      <translation>シェーディング</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="154"/>
      <source>Points</source>
      <translation>点群</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="162"/>
      <source>Shape color</source>
      <translation>シェイプの色</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="169"/>
      <source>The color of faces</source>
      <translation>面の色</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="176"/>
      <source>Transparency</source>
      <translation>透明度</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="183"/>
      <source>The transparency of faces</source>
      <translation>面の透明度</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="186"/>
      <source> %</source>
      <translation> %</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="199"/>
      <source>Annotations</source>
      <translation>注釈</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="205"/>
      <source>Text font</source>
      <translation>文字の書体</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="218"/>
      <source>The font to use for texts and dimensions</source>
      <translation>テキストと寸法に用いられるフォント</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="225"/>
      <source>Text size</source>
      <translation>文字サイズ</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="232"/>
      <source>The size of texts and dimension texts</source>
      <translation>テキストと寸法テキストのサイズ</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="242"/>
      <source>Text spacing</source>
      <translation>テキストの間隔</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="249"/>
      <source>The space between the text and the dimension line</source>
      <translation type="unfinished">The space between the text and the dimension line</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="259"/>
      <source>Text color</source>
      <translation>テキストの色</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="266"/>
      <source>The color of texts and dimension texts</source>
      <translation>テキストと寸法テキストの色</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="273"/>
      <source>Line spacing</source>
      <translation>行間</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="280"/>
      <source>The spacing between different lines of text</source>
      <translation type="unfinished">The spacing between different lines of text</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="287"/>
      <source>Arrow style</source>
      <translation>矢印の形状</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="294"/>
      <source>The type of dimension arrows</source>
      <translation>寸法矢印の種類</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="298"/>
      <source>Dot</source>
      <translation>点</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="303"/>
      <source>Circle</source>
      <translation>円</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="308"/>
      <source>Arrow</source>
      <translation>矢印</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="313"/>
      <source>Tick</source>
      <translation>目盛</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="318"/>
      <source>Tick-2</source>
      <translation>目盛り-2</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="326"/>
      <source>Arrow size</source>
      <translation>矢印のサイズ</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="333"/>
      <source>The size of dimension arrows</source>
      <translation>寸法矢印のサイズ</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="343"/>
      <source>Show unit</source>
      <translation>単位を表示</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="350"/>
      <source>If the unit suffix is shown on dimension texts or not</source>
      <translation>接尾の単位が寸法テキストに表示されているかどうか</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="363"/>
      <source>Unit override</source>
      <translation>単位の上書き</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="370"/>
      <source>The unit to use for dimensions. Leave blank to use current FreeCAD unit</source>
      <translation>寸法に使用する単位。現在のFreeCAD単位を使用する場合は空白のままにします。</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="382"/>
      <source>Apply above style to selected object(s)</source>
      <translation type="unfinished">Apply above style to selected object(s)</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="385"/>
      <source>Selected</source>
      <translation>選択</translation>
    </message>
    <message>
      <location filename="../ui/TaskPanel_SetStyle.ui" line="397"/>
      <source>Texts/dims</source>
      <translation type="unfinished">Texts/dims</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="14"/>
      <source>Form</source>
      <translation>フォーム</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="20"/>
      <source>PAT file:</source>
      <translation>PATファイル:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="27"/>
      <source>pattern files (*.pat)</source>
      <translation>パターンファイル (*.pat)</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="34"/>
      <source>Pattern:</source>
      <translation>パターン:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="44"/>
      <source>Scale</source>
      <translation>尺度</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="64"/>
      <source>Rotation:</source>
      <translation>回転:</translation>
    </message>
    <message>
      <location filename="../ui/dialogHatch.ui" line="71"/>
      <source>°</source>
      <translation>度</translation>
    </message>
  </context>
  <context>
    <name>Gui::Dialog::DlgSettingsDraft</name>
    <message>
      <location filename="../ui/preferences-draft.ui" line="14"/>
      <source>General settings</source>
      <translation>全般的な設定</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="32"/>
      <source>General Draft Settings</source>
      <translation>一般的なドラフトの設定</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="40"/>
      <source>Default working plane</source>
      <translation>デフォルトの作業平面</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="67"/>
      <source>None</source>
      <translation>なし</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="72"/>
      <source>XY (Top)</source>
      <translation>XY (平面図)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="77"/>
      <source>XZ (Front)</source>
      <translation>XZ (正面図)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="82"/>
      <source>YZ (Side)</source>
      <translation>YZ (側面図)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="94"/>
      <source>Internal precision level</source>
      <translation>内部の精度レベル</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="120"/>
      <source>The number of decimals in internal coordinates operations (for ex. 3 = 0.001). Values between 6 and 8 are usually considered the best trade-off among FreeCAD users.</source>
      <translation>内部での座標操作で使用される小数点以下の桁数（例. 3 = 0.001）。通常、FreeCAD ユーザーには6から8の間の値が 最も適しています。</translation>
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
      <translation>これは閾値を使用する機能で使用される値です。
この値よりも小さい差しかない値は同じものとして扱われます。上記の精度レベルが両方を制御するようになるとこの値は使用されなくなります。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="194"/>
      <source>If this option is checked, the layers drop-down list will also show groups, allowing you to automatically add objects to groups too.</source>
      <translation>このオプションにチェックが入っている場合、レイヤーのドロップダウンリストにもグループが表示され、自動的にオブジェクトをグループに追加することもできます。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="197"/>
      <source>Show groups in layers list drop-down button</source>
      <translation>ドロップダウンボタンのレイヤーリストでグループを表示</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="216"/>
      <source>Draft tools options</source>
      <translation>ドラフトツールのオプション</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="230"/>
      <source>When drawing lines, set focus on Length instead of X coordinate.
This allows to point the direction and type the distance.</source>
      <translation>線を描画する場合に、X座標の代わりに長さにフォーカスを設定します。
これにより、方向を指定して距離を入力できます。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="234"/>
      <source>Set focus on Length instead of X coordinate</source>
      <translation>X座標の代わりに、長さにフォーカスを設定</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="247"/>
      <source>Normally, after copying objects, the copies get selected.
If this option is checked, the base objects will be selected instead.</source>
      <translation>通常はオブジェクトをコピーした後にはコピーが選択されています。このオプションにチェックを入れると、代わりに元オブジェクトが選択されるようになります。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="251"/>
      <source>Select base objects after copying</source>
      <translation>コピー後に元オブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="264"/>
      <source>If this option is set, when creating Draft objects on top of an existing face of another object, the "Support" property of the Draft object will be set to the base object. This was the standard behaviour before FreeCAD 0.19</source>
      <translation>このオプションが設定されている場合、別のオブジェクトの既存面上にドラフト・オブジェクトを作成した時にドラフト・オブジェクトの「Support」プロパティーにベース・オブジェクトが設定されます。これは FreeCAD 0.19 以前には標準的な動作でした。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="267"/>
      <source>Set the Support property when possible</source>
      <translation>可能な場合にはサポート・プロパティーを設定</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="280"/>
      <source>If this is checked, objects will appear as filled by default.
Otherwise, they will appear as wireframe</source>
      <translation>チェックされている場合、オブジェクトはデフォルトと同様、塗りつぶし表示されます。それ以外の場合はワイヤーフレーム表示されます。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="284"/>
      <source>Fill objects with faces whenever possible</source>
      <translation>できるだけオブジェクトの面を塗りつぶす</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="300"/>
      <source>If this is checked, copy mode will be kept across command,
otherwise commands will always start in no-copy mode</source>
      <translation>チェックされている場合、別コマンドでもコピーモードが継続します。それ以外の場合、コマンドは常に非コピーモードで開始します。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="304"/>
      <source>Global copy mode</source>
      <translation>グローバルコピーモード</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="320"/>
      <source>Force Draft Tools to create Part primitives instead of Draft objects.
Note that this is not fully supported, and many object will be not editable with Draft Modifiers.</source>
      <translation>ドラフトツールがオブジェクトの代わりに部品プリミティブを強制的に作成します。
これは完全にはサポートされておらず、多くのオブジェクトはドラフトモディファイアでは編集できないことに注意してください。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="324"/>
      <source>Use Part Primitives when available</source>
      <translation>利用可能な場合はプリミティブ部品を使用</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="341"/>
      <source>Prefix labels of Clones with:</source>
      <translation>クローンの接頭ラベル：</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="372"/>
      <source>Construction Geometry</source>
      <translation>構築ジオメトリー</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="380"/>
      <source>Construction group name</source>
      <translation>構築グループ名</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="387"/>
      <source>This is the default group name for construction geometry</source>
      <translation>構築ジオメトリーのデフォルトグループ名です。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="390"/>
      <source>Construction</source>
      <translation>構築</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="407"/>
      <source>Construction geometry color</source>
      <translation>構築ジオメトリーの色</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draft.ui" line="427"/>
      <source>This is the default color for objects being drawn while in construction mode.</source>
      <translation>構築モード中に描画されるオブジェクトのデフォルトの色</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="14"/>
      <source>Visual settings</source>
      <translation>表示の設定</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="26"/>
      <source>Visual Settings</source>
      <translation>表示の設定</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="34"/>
      <source>Snap symbols style</source>
      <translation>スナップシンボルスタイル</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="54"/>
      <source>Draft classic style</source>
      <translation>Draftクラシックスタイル</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="59"/>
      <source>Bitsnpieces style</source>
      <translation>Bitsnpiecesスタイル</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="80"/>
      <source>Color</source>
      <translation>色</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="87"/>
      <source>The default color for snap symbols</source>
      <translation>スナップシンボルのデフォルトの色</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="111"/>
      <source>Check this if you want to use the color/linewidth from the toolbar as default</source>
      <translation>ツールバーの色/線幅をデフォルトとして使用したい場合はチェック</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="114"/>
      <source>Save current color and linewidth across sessions</source>
      <translation>セッションをまたいで現在の色と線幅を保存</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="131"/>
      <source>If checked, a widget indicating the current working plane orientation appears during drawing operations</source>
      <translation>チェックされている場合、製図操作中に現在の作業平面の方向を示すウィジットが表示されます。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="134"/>
      <source>Show Working Plane tracker</source>
      <translation>作業平面の追跡ツールを表示</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="151"/>
      <source>Default template sheet</source>
      <translation>デフォルトのテンプレートシート</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="177"/>
      <source>The default template to use when creating a new drawing sheet</source>
      <translation>デフォルトのテンプレートは、新規に図面シートを作成するときに使用する</translation>
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
      <translation>パターンを描画する解像度。デフォルト値は128です。値が大きいほど解像度が向上し、値が小さくなると描画が速くなります。</translation>
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
      <translation>ダウングレードやアップグレードで面の色を維持したい場合はチェック（splitFaces と makeShell のみ）</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="329"/>
      <source>Preserve colors of faces during downgrade/upgrade</source>
      <translation>ダウングレード/アップグレードで面の色を維持</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="346"/>
      <source>Check this if you want the face names to derive from the originating object name and vice versa while doing downgrade/upgrade (splitFaces and makeShell only)</source>
      <translation>元となるオブジェクトの名前から面の名前をつけたい場合、ダウングレード/アップグレード実行（splitFaces と makeShell のみ）でその逆を行いたい場合はチェック</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="349"/>
      <source>Preserve names of faces during downgrade/upgrade</source>
      <translation>ダウングレード/アップグレードで面の名前を維持</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="367"/>
      <source>Drawing view line definitions</source>
      <translation>ドローイング・ビュー・ラインの定義</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="375"/>
      <source>Dashed line definition</source>
      <translation>破線の定義</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="395"/>
      <location filename="../ui/preferences-draftvisual.ui" line="438"/>
      <location filename="../ui/preferences-draftvisual.ui" line="481"/>
      <source>An SVG linestyle definition</source>
      <translation>SVGのラインスタイルの定義</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="398"/>
      <source>0.09,0.05</source>
      <translation>0.09,0.05</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="418"/>
      <source>Dashdot line definition</source>
      <translation>鎖線の定義</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="441"/>
      <source>0.09,0.05,0.02,0.05</source>
      <translation>0.09,0.05,0.02,0.05</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="461"/>
      <source>Dotted line definition</source>
      <translation>点線の定義</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftvisual.ui" line="484"/>
      <source>0.02,0.02</source>
      <translation>0.02,0.02</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="14"/>
      <source>Texts and dimensions</source>
      <translation>テキストと寸法線</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="26"/>
      <source>Text settings</source>
      <translation>テキスト設定</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="34"/>
      <source>Font family</source>
      <translation>フォントの種類</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="47"/>
      <source>This is the default font name for all Draft texts and dimensions.
It can be a font name such as "Arial", a default style such as "sans", "serif"
or "mono", or a family such as "Arial,Helvetica,sans" or a name with a style
such as "Arial:Bold"</source>
      <translation>これは、すべてのドラフト・テキストと寸法に用いるデフォルト（省略時の設定）フォント名です。「Arial」のようなフォント名、「sans」,「serif」や「mono」のようなデフォルトのスタイル名、「Arial,Helvetica,sans」のような系統名、 「Arial:Bold」のようなフォント名とスタイル名を組み合わせた形で指定することができます。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="59"/>
      <source>Internal font</source>
      <translation>内部フォント</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="76"/>
      <source>Font size</source>
      <translation>フォントサイズ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="89"/>
      <source>Default height for texts and dimensions</source>
      <translation>テキストと寸法のデフォルトの高さ</translation>
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
      <translation>寸法設定</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="124"/>
      <source>Display mode</source>
      <translation>表示モード</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="144"/>
      <source>text above (2D)</source>
      <translation>ワークプレーン上(2D)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="149"/>
      <source> text inside (3D)</source>
      <translation>3Dビュー上(3D)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="161"/>
      <source>Number of decimals</source>
      <translation>小数点以下桁数</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="201"/>
      <source>Extension lines size</source>
      <translation>寸法補助線のサイズ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="208"/>
      <source>The default size of dimensions extension lines</source>
      <translation>寸法補助線のデフォルトサイズ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="237"/>
      <source>Extension line overshoot</source>
      <translation>寸法補助線の超過量</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="244"/>
      <source>The default length of extension line above dimension line</source>
      <translation>寸法線の上の寸法補助線のデフォルトの長さ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="273"/>
      <source>Dimension line overshoot</source>
      <translation>寸法線の超過量</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="280"/>
      <source>The default distance the dimension line is extended past extension lines</source>
      <translation>寸法線が寸法補助線を超えて伸びるデフォルトの距離</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="309"/>
      <source>Arrows style</source>
      <translation>矢印のスタイル</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="323"/>
      <source>Dot</source>
      <translation>点</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="328"/>
      <source>Circle</source>
      <translation>円</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="333"/>
      <source>Arrow</source>
      <translation>矢印</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="338"/>
      <source>Tick</source>
      <translation>目盛</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="343"/>
      <source>Tick-2</source>
      <translation>目盛り-2</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="355"/>
      <source>Arrows size</source>
      <translation>矢印のサイズ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="362"/>
      <source>The default size of arrows</source>
      <translation>矢印のデフォルトサイズ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="388"/>
      <source>Text orientation</source>
      <translation>テキストの向き</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="395"/>
      <source>This is the orientation of the dimension texts when those dimensions are vertical. Default is left, which is the ISO standard.</source>
      <translation>寸法が縦になったときの寸法テキストの向き。デフォルトは左（ISO標準）</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="405"/>
      <source>Left (ISO standard)</source>
      <translation>左 (ISO規格)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="410"/>
      <source>Right</source>
      <translation>右面図</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="422"/>
      <source>Text spacing</source>
      <translation>テキストの間隔</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="429"/>
      <source>The space between the dimension line and the dimension text</source>
      <translation>寸法線と寸法テキストの間隔</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="455"/>
      <source>Show the unit suffix in dimensions</source>
      <translation>寸法の単位サフィックスを表示</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="475"/>
      <source>Override unit</source>
      <translation>単位を上書き</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="495"/>
      <source>By leaving this field blank, the dimension measurements will be shown in the current unit defined in FreeCAD. By indicating a unit here such as m or cm, you can force new dimensions to be shown in that unit.</source>
      <translation>この入力欄を空白のままにすると、FreeCADで定義されている現在の単位で寸法測定値が表示されます。
ここでmやcmなどの単位を指定すると、その単位で新しい寸法が表示されるようにできます。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="513"/>
      <source>ShapeString settings</source>
      <translation>シェイプストリング設定</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="521"/>
      <source>Default ShapeString font file</source>
      <translation>デフォルトのシェイプストリング用フォントファイル</translation>
    </message>
    <message>
      <location filename="../ui/preferences-drafttexts.ui" line="534"/>
      <source>Select a font file</source>
      <translation>フォントファイルを選択</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="14"/>
      <source>SVG</source>
      <translation>SVG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="40"/>
      <source>Import style</source>
      <translation>インポートスタイル</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="47"/>
      <source>Method chosen for importing SVG object color to FreeCAD</source>
      <translation>SVGオブジェクトの色をFreeCADにインポートするために選択されたメソッド</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="60"/>
      <source>None (fastest)</source>
      <translation>使用しない（最速）</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="65"/>
      <source>Use default color and linewidth</source>
      <translation>デフォルトの色と線幅を使用する</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="70"/>
      <source>Original color and linewidth</source>
      <translation>標準の色と線幅</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="82"/>
      <source>If checked, no units conversion will occur.
One unit in the SVG file will translate as one millimeter. </source>
      <translation>ここにチェックが入っている場合、単位変換を行いません。
SVG ファイル内の 1 単位は、1 ミリメートルとして変換されます。 </translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="86"/>
      <source>Disable units scaling</source>
      <translation>単位のスケーリングを無効</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="121"/>
      <source>Export style</source>
      <translation>エクスポートスタイル</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="128"/>
      <source>Style of SVG file to write when exporting a sketch</source>
      <translation>スケッチをエクスポートする際に、SVGファイルのスタイルも書き出す</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="141"/>
      <source>Translated (for print &amp; display)</source>
      <translation>変換 (印刷&amp;表示用)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="146"/>
      <source>Raw (for CAM)</source>
      <translation>変換無し (CAM用)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="158"/>
      <source>All white lines will appear in black in the SVG for better readability against white backgrounds</source>
      <translation>すべての白い線は白い背景に対する読みやすさを向上させるため、SVGでは黒として表示されます</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="161"/>
      <source>Translate white line color to black</source>
      <translation>白い線の色を黒に変換</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="181"/>
      <source>Max segment length for discretized arcs</source>
      <translation>円弧の離散化時の最大セグメント長さ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-svg.ui" line="204"/>
      <source>Versions of Open CASCADE older than version 6.8 don't support arc projection.
In this case arcs will be discretized into small line segments.
This value is the maximum segment length. </source>
      <translation>Open CASCADEのバージョンが6.8よりも古いバージョンだと、円弧の投影をサポートしていません。
この場合、円弧は小さな線分に分割されます。
この値は最大セグメント長です。 </translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="14"/>
      <source>OCA</source>
      <translation>OCA</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="46"/>
      <source>Check this if you want the areas (3D faces) to be imported too.</source>
      <translation>領域(3D面)もインポートしたい場合はチェックしてください。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="49"/>
      <source>Import OCA areas</source>
      <translation>OCA領域をインポート</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="14"/>
      <source>DXF</source>
      <translation>DXF</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="35"/>
      <source>This preferences dialog will be shown when importing/ exporting DXF files</source>
      <translation>この設定ダイアログは、DXFファイルのインポート／エクスポート時に表示されます</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="38"/>
      <source>Show this dialog when importing and exporting</source>
      <translation>インポート、エクスポートするときにこのダイアログボックスを表示</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="51"/>
      <source>Python importer is used, otherwise the newer C++ is used.
Note: C++ importer is faster, but is not as featureful yet</source>
      <translation>Pythonのimporterが使用されます。そうでなければ、新しいC++が使用されます。
注意: C++のimporterは速いですが、機能的ではありません。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="55"/>
      <source>Use legacy python importer</source>
      <translation>従来型の Python インポーターを使用</translation>
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
      <translation>従来型の Python エクスポーターを使用</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="88"/>
      <source>Automatic update (legacy importer only)</source>
      <translation>自動更新 (旧インポート機能のみ)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="96"/>
      <source>Allow FreeCAD to download the Python converter for DXF import and export.
You can also do this manually by installing the "dxf_library" workbench
from the Addon Manager.</source>
      <translation>DXFのインポートとエクスポートを行うPythonコンバータのダウンロードをFreeCADに許可します。
アドオンマネージャーから「dxf_library」ワークベンチ
をインストールして手動で行うこともできます。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="101"/>
      <source>Allow FreeCAD to automatically download and update the DXF libraries</source>
      <translation>DXF ライブラリの自動的なダウンロードと更新を FreeCAD に許可</translation>
    </message>
    <message>
      <location filename="../ui/preferences-oca.ui" line="26"/>
      <location filename="../ui/preferences-dxf.ui" line="119"/>
      <location filename="../ui/preferences-svg.ui" line="26"/>
      <source>Import options</source>
      <translation>インポート・オプション</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="140"/>
      <source>Note: Not all the options below are used by the new importer yet</source>
      <translation>注: 以下のオプションには新しいインポート機能ではまだ使用できないものがあります</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="149"/>
      <source>Import</source>
      <translation>インポート</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="156"/>
      <source>If unchecked, texts and mtexts won't be imported</source>
      <translation>ここにチェックが入っていない場合、テキストとmtextはインポートされません</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="159"/>
      <source>texts and dimensions</source>
      <translation>テキストと寸法線</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="172"/>
      <source>If unchecked, points won't be imported</source>
      <translation>ここにチェックが入っていない場合、ポイントはインポートされません</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="175"/>
      <source>points</source>
      <translation>ポイント</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="188"/>
      <source>If checked, paper space objects will be imported too</source>
      <translation>チェックが入っている場合、ペーパースペースオブジェクトもインポートされます</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="191"/>
      <source>layouts</source>
      <translation>レイアウト</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="204"/>
      <source>If you want the non-named blocks (beginning with a *) to be imported too</source>
      <translation>（*で始まる）名称のないブロックも一緒にインポートしたい場合は、ここにチェックを入れて下さい</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="207"/>
      <source>*blocks</source>
      <translation>*blocks</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="224"/>
      <source>Create</source>
      <translation>作成</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="231"/>
      <source>Only standard Part objects will be created (fastest)</source>
      <translation>標準のPartオブジェクトだけが作成されます(最も高速)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="234"/>
      <source>simple Part shapes</source>
      <translation>パート形状を単純化</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="250"/>
      <source>Parametric Draft objects will be created whenever possible</source>
      <translation>できる限りパラメトリックなドラフト・オブジェクトが作成されます</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="253"/>
      <source>Draft objects</source>
      <translation>ドラフトオブジェクト</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="266"/>
      <source>Sketches will be created whenever possible</source>
      <translation>できる限りスケッチが作成します</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="269"/>
      <source>Sketches</source>
      <translation>スケッチ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="289"/>
      <source>Scale factor to apply to imported files</source>
      <translation>インポートされたファイルに適用する尺度の係数</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="309"/>
      <source>Scale factor to apply to DXF files on import.
The factor is the conversion between the unit of your DXF file and millimeters.
Example: for files in millimeters: 1, in centimeters: 10,
                             in meters: 1000, in inches: 25.4, in feet: 304.8</source>
      <translation>DXFファイルのインポート時に適用する尺度の係数。
係数は、DXFファイルの単位をミリメートルに変換するために用いられます。
例：ファイルでの単位が、ミリメートルの場合に 1、センチメートルならば 10、
メートルならば 1000 、インチならば 2.54、フィートならば 304.8 となります。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="338"/>
      <source>Colors will be retrieved from the DXF objects whenever possible.
Otherwise default colors will be applied. </source>
      <translation>色は可能な限りDXFオブジェクトから読み出されます。それ以外の場合、デフォルトの色が適用されます。 </translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="342"/>
      <source>Get original colors from the DXF file</source>
      <translation>DXFファイルからオリジナルの色を取得</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="359"/>
      <source>FreeCAD will try to join coincident objects into wires.
Note that this can take a while!</source>
      <translation>FreeCADは一致するオブジェクトをワイヤーに結合しようとします。これには時間がかかる場合があります。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="363"/>
      <source>Join geometry</source>
      <translation>ジオメトリーを結合</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="380"/>
      <source>Objects from the same layers will be joined into Draft Blocks,
turning the display faster, but making them less easily editable </source>
      <translation>同じレイヤーのオブジェクトがドラフトブロックとして連結されて表示が高速になりますが、編集は難しくなります。 </translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="384"/>
      <source>Group layers into blocks</source>
      <translation>グループレイヤーをブロックへ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="401"/>
      <source>Imported texts will get the standard Draft Text size,
instead of the size they have in the DXF document</source>
      <translation>インポートされたテキストは、DXF 文書内にあるサイズからドラフトで定義された標準のテキストサイズへ変換されます。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="405"/>
      <source>Use standard font size for texts</source>
      <translation>標準フォントサイズを使用</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="422"/>
      <source>If this is checked, DXF layers will be imported as Draft Layers</source>
      <translation>ここがチェックされている場合、DXFレイヤーはドラフトレイヤーとしてインポートされます</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="425"/>
      <source>Use Layers</source>
      <translation>レイヤーを使用</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="445"/>
      <source>Hatches will be converted into simple wires</source>
      <translation>ハッチングは単純なワイヤーに変換されます</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="448"/>
      <source>Import hatch boundaries as wires</source>
      <translation>ハッチング境界をワイヤーとしてインポート</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="465"/>
      <source>If polylines have a width defined, they will be rendered
as closed wires with correct width</source>
      <translation>ポリラインに幅が定義されている場合、正適切な幅の閉じたワイヤーとしてレンダリングされます</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="469"/>
      <source>Render polylines with width</source>
      <translation>幅のあるポリラインをレンダリング</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="486"/>
      <source>Ellipse export is poorly supported. Use this to export them as polylines instead.</source>
      <translation>楕円のエクスポートは十分にサポートされていません。代わりにこれを使用してポリラインとしてエクスポートしてください。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="489"/>
      <source>Treat ellipses and splines as polylines</source>
      <translation>楕円およびスプラインをポリラインとして扱う</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="518"/>
      <source>Max Spline Segment:</source>
      <translation>スプラインの最大セグメント数:</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="528"/>
      <source>Maximum length of each of the polyline segments.
If it is set to '0' the whole spline is treated as a straight segment.</source>
      <translation>各ポリラインセグメントの最大長さ。
'0' に設定すると、スプライン全体が直線セグメントとして扱われます。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="559"/>
      <location filename="../ui/preferences-svg.ui" line="107"/>
      <source>Export options</source>
      <translation>エクスポート・オプション</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="567"/>
      <source>All objects containing faces will be exported as 3D polyfaces</source>
      <translation>面を含むすべてのオブジェクトは 3D ポリフェースとしてエクスポートされます</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="570"/>
      <source>Export 3D objects as polyface meshes</source>
      <translation>3D オブジェクトをポリフェイスメッシュとしてエクスポート</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="587"/>
      <source>Drawing Views will be exported as blocks.
This might fail for post DXF R12 templates.</source>
      <translation>ドローイング・ビューはブロックとしてエクスポートされます。
DXF R12 後のテンプレートでは失敗する可能性があります。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="591"/>
      <source>Export Drawing Views as blocks</source>
      <translation>ドローイングビューをブロックとしてエクスポート</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="611"/>
      <source>Exported objects will be projected to reflect the current view direction</source>
      <translation>エクスポートされたオブジェクトは現在のビュー方向を反映するように投影されます</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dxf.ui" line="614"/>
      <source>Project exported objects along current view direction</source>
      <translation>エクスポートされるオブジェクトを現在のビュー方向と平行に投影</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="14"/>
      <source>Grid and snapping</source>
      <translation>グリッドとスナップ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="35"/>
      <source>Snapping</source>
      <translation>スナップ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="43"/>
      <source>If this is checked, snapping is activated without the need to press the snap mod key</source>
      <translation>ここをチェックした場合、スナップはモディキーを押さなくてもスナップする</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="46"/>
      <source>Always snap (disable snap mod)</source>
      <translation>常にスナップ (スナップモードを無効にする)</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="66"/>
      <source>Constrain mod</source>
      <translation>拘束モード</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="86"/>
      <source>The Constraining modifier key</source>
      <translation>拘束の修飾キー</translation>
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
      <translation>スナップモード</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="138"/>
      <source>The snap modifier key</source>
      <translation>スナップの修飾キー</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="173"/>
      <source>Alt mod</source>
      <translation>Altモード</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="193"/>
      <source>The Alt modifier key</source>
      <translation>Alt修飾キー</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="228"/>
      <source>If checked, the Snap toolbar will be shown whenever you use snapping</source>
      <translation>チェックされている場合、スナップ使用時に常にスナップツールバーが表示されます</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="231"/>
      <source>Show Draft Snap toolbar</source>
      <translation>ドラフトスナップ ツールバーを表示</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="251"/>
      <source>Hide Draft snap toolbar after use</source>
      <translation>使用後ドラフトスナップ ツールバーを非表示にする</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="272"/>
      <source>Grid</source>
      <translation>グリッド</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="278"/>
      <source>If checked, a grid will appear when drawing</source>
      <translation>チェックした場合、製図中にグリッドが表示されます</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="281"/>
      <source>Use grid</source>
      <translation>グリッドを使用</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="300"/>
      <source>If checked, the Draft grid will always be visible when the Draft workbench is active. Otherwise only when using a command</source>
      <translation>チェックされている場合、ドラフトワークベンチのアクティブ状態中に常にドラフトグリッドが表示されます。それ以外の場合はコマンド使用時のみです。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="303"/>
      <source>Always show the grid</source>
      <translation>常にグリッドを表示</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="319"/>
      <source>If checked, an additional border is displayed around the grid, showing the main square size in the bottom left border</source>
      <translation>ここにチェックが入っている場合に、追加の境界線がグリッド周りに表示され、左下境界線で主な正方形を表示します</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="322"/>
      <source>Show grid border</source>
      <translation>グリッド線を表示</translation>
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
      <translation>設定されている場合、グリッドは赤、緑、青の 2 つの主軸をグローバル軸に一致させます。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="360"/>
      <source>Use colored axes</source>
      <translation>色の軸を使用</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="381"/>
      <source>Main lines every</source>
      <translation>主線の間隔</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="404"/>
      <source>Mainlines will be drawn thicker. Specify here how many squares between mainlines.</source>
      <translation>主線は太めに描画されます。主線間を何スクエアにするかここに指定してください。</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="430"/>
      <source>Grid spacing</source>
      <translation>グリッド間隔</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="453"/>
      <source>The spacing between each grid line</source>
      <translation>グリッド線の間隔</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="485"/>
      <source>Grid size</source>
      <translation>グリッドサイズ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="505"/>
      <source>The number of horizontal or vertical lines of the grid</source>
      <translation>グリッドの水平線または垂直線の数</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="511"/>
      <source> lines</source>
      <translation> 線</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="534"/>
      <source>Grid color and transparency</source>
      <translation>グリッドの色と透明度</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="554"/>
      <source>The color of the grid</source>
      <translation>グリッドの色</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="574"/>
      <source>The overall transparency of the grid</source>
      <translation>グリッドの全体的な透明度</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="595"/>
      <source>Draft Edit preferences</source>
      <translation>ドラフト編集の設定</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="598"/>
      <source>Edit</source>
      <translation>編集</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="621"/>
      <source>Maximum number of contemporary edited objects</source>
      <translation>同時に編集できるオブジェクトの最大数</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="644"/>
      <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Sets the maximum number of objects Draft Edit&lt;/p&gt;&lt;p&gt;can process at the same time&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;ドラフト編集で同時に処理&lt;/p&gt;&lt;p&gt;できるオブジェクトの数&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="691"/>
      <source>Draft edit pick radius</source>
      <translation>ピックしている半径を編集</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftsnap.ui" line="714"/>
      <source>Controls pick radius of edit nodes</source>
      <translation>編集ノードのピックしている半径を制御します</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="14"/>
      <source>DWG</source>
      <translation>DWG</translation>
    </message>
    <message>
      <location filename="../ui/preferences-dwg.ui" line="26"/>
      <source>DWG conversion</source>
      <translation>DWG 変換</translation>
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
      <translation>自動</translation>
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
      <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=" font-weight:600;"&gt;無し:&lt;/span&lt;head/&gt;&gt; DXFオプションはWGファイルにも適用されます。&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="14"/>
      <source>User interface settings</source>
      <translation>ユーザーインターフェイス設定</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="26"/>
      <source>In-Command Shortcuts</source>
      <translation>コマンド内ショートカット</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="37"/>
      <source>Relative</source>
      <translation>相対</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="59"/>
      <source>R</source>
      <translation>R</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="81"/>
      <source>Continue</source>
      <translation>続行</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="103"/>
      <source>T</source>
      <translation>T</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="125"/>
      <source>Close</source>
      <translation>閉じる</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="147"/>
      <source>O</source>
      <translation>O</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="169"/>
      <source>Copy</source>
      <translation>コピー</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="191"/>
      <source>P</source>
      <translation>P</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="213"/>
      <source>Subelement Mode</source>
      <translation>サブ要素モード</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="235"/>
      <source>D</source>
      <translation>D</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="257"/>
      <source>Fill</source>
      <translation>塗りつぶし</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="279"/>
      <source>L</source>
      <translation>L</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="301"/>
      <source>Exit</source>
      <translation>終了</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="323"/>
      <source>A</source>
      <translation>A</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="345"/>
      <source>Select Edge</source>
      <translation>エッジを選択</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="367"/>
      <source>E</source>
      <translation>E</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="389"/>
      <source>Add Hold</source>
      <translation>ホールドを追加</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="411"/>
      <source>Q</source>
      <translation>Q</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="433"/>
      <source>Length</source>
      <translation>長さ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="455"/>
      <source>H</source>
      <translation>H</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="477"/>
      <source>Wipe</source>
      <translation>ワイプ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="499"/>
      <source>W</source>
      <translation>W</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="521"/>
      <source>Set WP</source>
      <translation>WPを設定</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="543"/>
      <source>U</source>
      <translation>U</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="565"/>
      <source>Cycle Snap</source>
      <translation>サイクル・スナップ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="587"/>
      <source>`</source>
      <translation>`</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="609"/>
      <source>Global</source>
      <translation>全体</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="631"/>
      <source>G</source>
      <translation type="unfinished">G</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="653"/>
      <source>Snap</source>
      <translation>スナップ</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="675"/>
      <source>S</source>
      <translation>S</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="697"/>
      <source>Increase Radius</source>
      <translation>半径を増加</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="719"/>
      <source>[</source>
      <translation>[</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="741"/>
      <source>Decrease Radius</source>
      <translation>半径を減少</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="763"/>
      <source>]</source>
      <translation>]</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="785"/>
      <source>Restrict X</source>
      <translation>X 制限</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="807"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="829"/>
      <source>Restrict Y</source>
      <translation>Y 制限</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="851"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="873"/>
      <source>Restrict Z</source>
      <translation>Z 制限</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="895"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="928"/>
      <source>Enable draft statusbar customization</source>
      <translation>ドラフト・ステータスバーのカスタマイズを有効にする</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="931"/>
      <source>Draft Statusbar</source>
      <translation>ドラフトのステータス・バー</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="951"/>
      <source>Enable snap statusbar widget</source>
      <translation>ステータスバー・ウィンジェットのスナップを有効にする</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="954"/>
      <source>Draft snap widget</source>
      <translation>ドラフトをウィンジェットにスナップさせる</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="970"/>
      <source>Enable draft statusbar annotation scale widget</source>
      <translation>ドラフトのステータスバーで尺度の注釈ウィンジェットを有効にする</translation>
    </message>
    <message>
      <location filename="../ui/preferences-draftinterface.ui" line="973"/>
      <source>Annotation scale widget</source>
      <translation>尺度の注釈ウィンジェット</translation>
    </message>
  </context>
  <context>
    <name>ImportAirfoilDAT</name>
    <message>
      <location filename="../../importAirfoilDAT.py" line="193"/>
      <source>Did not find enough coordinates</source>
      <translation>十分な座標が見つかりませんでした</translation>
    </message>
  </context>
  <context>
    <name>ImportSVG</name>
    <message>
      <location filename="../../importSVG.py" line="1796"/>
      <source>Unknown SVG export style, switching to Translated</source>
      <translation>未知の SVG エクスポートスタイルです。変換モードに切り替えます。</translation>
    </message>
    <message>
      <location filename="../../importSVG.py" line="1816"/>
      <source>The export list contains no object with a valid bounding box</source>
      <translation>エクスポート対象リストに有効なバウンディングボックスを持つオブジェクトが含まれていません。</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../InitGui.py" line="104"/>
      <source>Draft creation tools</source>
      <translation>ドラフト作成ツール</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="107"/>
      <source>Draft annotation tools</source>
      <translation>ドラフト注釈ツール</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="110"/>
      <source>Draft modification tools</source>
      <translation>ドラフト修正ツール</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="113"/>
      <source>Draft utility tools</source>
      <translation>ドラフトユーティリティツール</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="118"/>
      <source>&amp;Drafting</source>
      <translation>2次元製図 (&amp;D)</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="121"/>
      <source>&amp;Annotation</source>
      <translation>注釈(&amp;A)</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="124"/>
      <source>&amp;Modification</source>
      <translation>変更(&amp;M)</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="127"/>
      <source>&amp;Utilities</source>
      <translation>ユーティリティ (&amp;U)</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="50"/>
      <source>Arc tools</source>
      <translation>円弧ツール</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="58"/>
      <source>Bézier tools</source>
      <translation type="unfinished">Bézier tools</translation>
    </message>
    <message>
      <location filename="../../draftutils/init_tools.py" line="89"/>
      <source>Array tools</source>
      <translation>整列ツール</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snapper.py" line="1477"/>
      <source>Draft Snap</source>
      <translation>ドラフトスナップ</translation>
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
      <translation>DXF形式を処理するためにFreeCADが必要とするDXFインポート/エクスポート用ライブラリーがシステム内に見つかりません。
FreeCADでこれら機能を有効にするために、以下のいずれかの方法でライブラリをダウンロードして下さい：
1 - Draftワークベンチを読み込み
2 - メニューの 編集＞設定＞インポート-エクスポート＞DXF ＞ダウンロードを有効にする
またはライブラリー
https://github.com/yorikvanhavre/Draft-dxf-importer
の説明に従ってを手動でダウンロードします。
FreeCADでこれらライブラリーのダウンロードを有効にするためにYesを選択します。</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="57"/>
      <location filename="../../DraftGui.py" line="751"/>
      <source>Relative</source>
      <translation>相対</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="61"/>
      <location filename="../../DraftGui.py" line="756"/>
      <source>Global</source>
      <translation>全体</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="66"/>
      <location filename="../../DraftGui.py" line="774"/>
      <location filename="../../DraftGui.py" line="1126"/>
      <source>Continue</source>
      <translation>続行</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="71"/>
      <location filename="../../DraftGui.py" line="790"/>
      <source>Close</source>
      <translation>閉じる</translation>
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
      <translation>コピー</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="81"/>
      <source>Subelement mode</source>
      <translation>サブ要素モード</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="86"/>
      <source>Fill</source>
      <translation>塗りつぶし</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="91"/>
      <source>Exit</source>
      <translation>終了</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="96"/>
      <source>Snap On/Off</source>
      <translation>スナップのオン/オフ</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="101"/>
      <source>Increase snap radius</source>
      <translation>スナップ半径を増やす</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="106"/>
      <source>Decrease snap radius</source>
      <translation>スナップ半径を減らす</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="111"/>
      <source>Restrict X</source>
      <translation>X 制限</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="116"/>
      <source>Restrict Y</source>
      <translation>Y 制限</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="121"/>
      <source>Restrict Z</source>
      <translation>Z 制限</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="126"/>
      <location filename="../../DraftGui.py" line="796"/>
      <source>Select edge</source>
      <translation>エッジを選択</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="131"/>
      <source>Add custom snap point</source>
      <translation>カスタムスナップポイントを追加</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="136"/>
      <source>Length mode</source>
      <translation>長さモード</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="141"/>
      <location filename="../../DraftGui.py" line="792"/>
      <source>Wipe</source>
      <translation>ワイプ</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="146"/>
      <source>Set Working Plane</source>
      <translation>作業平面を設定</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="151"/>
      <source>Cycle snap object</source>
      <translation>循環的にオブジェクトにスナップ</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="156"/>
      <source>Toggle near snap on/off</source>
      <translation>近くでスナップさせる／させないの切り替え</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="330"/>
      <source>Draft Command Bar</source>
      <translation>ドラフトコマンドバー</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="659"/>
      <location filename="../../WorkingPlane.py" line="821"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="374"/>
      <source>Top</source>
      <translation>上面図</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="661"/>
      <location filename="../../WorkingPlane.py" line="832"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="386"/>
      <source>Front</source>
      <translation>正面図</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="663"/>
      <location filename="../../WorkingPlane.py" line="843"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="398"/>
      <source>Side</source>
      <translation>サイド</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="665"/>
      <source>Auto</source>
      <translation>自動</translation>
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
      <translation>なし</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="728"/>
      <source>active command:</source>
      <translation>アクティブコマンド:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="730"/>
      <source>Active Draft command</source>
      <translation>アクティブドラフトコマンド</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="731"/>
      <source>X coordinate of next point</source>
      <translation>次の点の X 座標</translation>
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
      <translation>次の点の Y 座標</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="736"/>
      <source>Z coordinate of next point</source>
      <translation>次の点の Z 座標</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="737"/>
      <source>Enter point</source>
      <translation>点の入力</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="739"/>
      <source>Enter a new point with the given coordinates</source>
      <translation>指定された座標に新しい点を入力</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="740"/>
      <source>Length</source>
      <translation>長さ</translation>
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
      <translation>現在のセグメントの長さ</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="743"/>
      <source>Angle of current segment</source>
      <translation>現在のセグメントの角度</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="747"/>
      <source>Check this to lock the current angle</source>
      <translation>ここにチェックを入れると、現在の角度を拘束</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="748"/>
      <location filename="../../DraftGui.py" line="1108"/>
      <source>Radius</source>
      <translation>半径</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="749"/>
      <location filename="../../DraftGui.py" line="1109"/>
      <source>Radius of Circle</source>
      <translation>円の半径</translation>
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
      <translation>塗りつぶし</translation>
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
      <translation>完了</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="769"/>
      <source>Finishes the current drawing or editing operation</source>
      <translation>現在の製図、または編集操作を終了</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="772"/>
      <source>If checked, command will not finish until you press the command button again</source>
      <translation>チェックされている場合、再びコマンドボタンを押すまでコマンドが終了しなくなります</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="777"/>
      <source>If checked, an OCC-style offset will be performedinstead of the classic offset</source>
      <translation type="unfinished">If checked, an OCC-style offset will be performedinstead of the classic offset</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="778"/>
      <source>&amp;OCC-style offset</source>
      <translation>OCC スタイル オフセット (&amp;O)</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="788"/>
      <source>&amp;Undo (CTRL+Z)</source>
      <translation>取り消し(CTRL+Z) (&amp;U)</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="789"/>
      <source>Undo the last segment</source>
      <translation>最後のセグメントを元に戻す</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="791"/>
      <source>Finishes and closes the current line</source>
      <translation>現在の線を閉じて終了</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="793"/>
      <source>Wipes the existing segments of this line and starts again from the last point</source>
      <translation>直線の既存のセグメントを消去して最後の点から再開</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="794"/>
      <source>Set WP</source>
      <translation>WPを設定</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="795"/>
      <source>Reorients the working plane on the last segment</source>
      <translation>最後のセグメントで作業平面を再設定</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="797"/>
      <source>Selects an existing edge to be measured by this dimension</source>
      <translation>寸法を計測したいところの既存エッジを選択して下さい</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="798"/>
      <source>Sides</source>
      <translation>側面</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="799"/>
      <source>Number of sides</source>
      <translation>辺の数</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="802"/>
      <source>If checked, objects will be copied instead of moved. Preferences -&gt; Draft -&gt; Global copy mode to keep this mode in next commands</source>
      <translation>チェックされている場合、オブジェクトは移動の代わりにコピーされます。設定 → Draft → グローバルコピーモード で次回以降のコマンドに対してこのモードを維持します。</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="803"/>
      <source>Modify subelements</source>
      <translation>サブ要素を変更</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="804"/>
      <source>If checked, subelements will be modified instead of entire objects</source>
      <translation>チェックされている場合、オブジェクト全体ではなくサブ要素が変更されます。</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="805"/>
      <source>Text string to draw</source>
      <translation>描画するテキスト文字列</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="806"/>
      <source>String</source>
      <translation>文字列</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="807"/>
      <source>Height of text</source>
      <translation>テキストの高さ</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="808"/>
      <source>Height</source>
      <translation>高さ</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="809"/>
      <source>Intercharacter spacing</source>
      <translation>文字間隔</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="810"/>
      <source>Tracking</source>
      <translation>トラッキング</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="811"/>
      <source>Full path to font file:</source>
      <translation>フォントファイルのフルパス:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="812"/>
      <source>Open a FileChooser for font file</source>
      <translation>ファイル選択ダイアログを開いてフォントファイルを選択</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="813"/>
      <source>Create text</source>
      <translation>テキストを作成</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="814"/>
      <source>Press this button to create the text object, or finish your text with two blank lines</source>
      <translation>このボタンを押して、テキストオブジェクト作成するか、２行の空白行でテキストを終了します。</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="836"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="272"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="327"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="530"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="541"/>
      <source>Current working plane</source>
      <translation>現在の作業平面</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="837"/>
      <source>Change default style for new objects</source>
      <translation>新しいオブジェクトの既定のスタイルを変更する</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="838"/>
      <source>Toggle construction mode</source>
      <translation>構築モードの切り替え</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="839"/>
      <location filename="../../DraftGui.py" line="2050"/>
      <location filename="../../DraftGui.py" line="2065"/>
      <source>Autogroup off</source>
      <translation>オートグループ無効</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="950"/>
      <source>Line</source>
      <translation>直線</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="958"/>
      <source>DWire</source>
      <translation>Dワイヤー</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="976"/>
      <source>Circle</source>
      <translation>円</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="981"/>
      <source>Arc</source>
      <translation>円弧</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="986"/>
      <location filename="../../draftguitools/gui_rotate.py" line="286"/>
      <source>Rotate</source>
      <translation>回転</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="990"/>
      <source>Point</source>
      <translation>点</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1018"/>
      <source>Label</source>
      <translation>ラベル</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1036"/>
      <location filename="../../draftguitools/gui_selectplane.py" line="527"/>
      <location filename="../../draftguitools/gui_offset.py" line="243"/>
      <location filename="../../draftguitools/gui_offset.py" line="260"/>
      <location filename="../../draftguitools/gui_offset.py" line="324"/>
      <source>Offset</source>
      <translation>オフセット</translation>
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
      <translation>オフセットの距離</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1097"/>
      <source>Trimex</source>
      <translation>トリメックス</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1197"/>
      <source>Pick Object</source>
      <translation>オブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1203"/>
      <source>Edit</source>
      <translation>編集</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1253"/>
      <source>Local u0394X</source>
      <translation type="unfinished">Local u0394X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1254"/>
      <source>Local u0394Y</source>
      <translation type="unfinished">Local u0394Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1255"/>
      <source>Local u0394Z</source>
      <translation type="unfinished">Local u0394Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1257"/>
      <source>Local X</source>
      <translation>ローカル座標 X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1258"/>
      <source>Local Y</source>
      <translation>ローカル座標 Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1259"/>
      <source>Local Z</source>
      <translation>ローカル座標 Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1261"/>
      <source>Global u0394X</source>
      <translation type="unfinished">Global u0394X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1262"/>
      <source>Global u0394Y</source>
      <translation type="unfinished">Global u0394Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1263"/>
      <source>Global u0394Z</source>
      <translation type="unfinished">Global u0394Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1265"/>
      <source>Global X</source>
      <translation>グローバル座標 X</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1266"/>
      <source>Global Y</source>
      <translation>グローバル座標 Y</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1267"/>
      <source>Global Z</source>
      <translation>グローバル座標 Z</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1503"/>
      <source>Invalid Size value. Using 200.0.</source>
      <translation>サイズの値が無効です。　200.0 を使ってください。</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1511"/>
      <source>Invalid Tracking value. Using 0.</source>
      <translation>トラッキングの値が無効です。　0 を使ってください。</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1525"/>
      <source>Please enter a text string.</source>
      <translation>文字を入力してください。</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1534"/>
      <source>Select a Font file</source>
      <translation>フォントファイルを選択</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="1567"/>
      <source>Please enter a font file.</source>
      <translation>フォントファイルを入力してください。</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2058"/>
      <source>Autogroup:</source>
      <translation type="unfinished">Autogroup:</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2394"/>
      <source>Faces</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2395"/>
      <source>Remove</source>
      <translation>削除</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2396"/>
      <source>Add</source>
      <translation>追加</translation>
    </message>
    <message>
      <location filename="../../DraftGui.py" line="2397"/>
      <source>Facebinder elements</source>
      <translation>フェイスバインダー要素</translation>
    </message>
    <message>
      <location filename="../../InitGui.py" line="46"/>
      <source>Draft</source>
      <translation>喫水</translation>
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
      <translation>変換中:</translation>
    </message>
    <message>
      <location filename="../../importDWG.py" line="223"/>
      <source>Conversion successful</source>
      <translation>変換に成功しました</translation>
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
      <translation>シェイプは全て同一平面上にある必要があります。</translation>
    </message>
    <message>
      <location filename="../../WorkingPlane.py" line="721"/>
      <source>Selected Shapes must define a plane</source>
      <translation>選択したシェイプは平面を定義する必要があります</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="81"/>
      <source>No graphical interface</source>
      <translation type="unfinished">No graphical interface</translation>
    </message>
    <message>
      <location filename="../../draftutils/gui_utils.py" line="161"/>
      <source>Unable to insert new object into a scaled part</source>
      <translation>拡大縮小したパートに新しいオブジェクトを挿入できません。</translation>
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
      <translation>アクティブなドキュメントがありません。中止しています。</translation>
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
      <translation>間違った入力：オブジェクトがドキュメント内にありません。</translation>
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
      <translation>カスタム</translation>
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
      <translation>x:x、x=x形式でカスタム注釈拡大縮小率を設定</translation>
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
      <translation>面:</translation>
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
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../draftutils/utils.py" line="663"/>
      <source>Wire</source>
      <translation>ワイヤー</translation>
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
      <translation>ドラフトグリッドの切り替え</translation>
    </message>
    <message>
      <location filename="../../draftobjects/shapestring.py" line="69"/>
      <source>ShapeString: string has no wires</source>
      <translation>シェイプストリング：文字列にワイヤーがありません</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="89"/>
      <location filename="../../draftobjects/draft_annotation.py" line="105"/>
      <source>added view property 'ScaleMultiplier'</source>
      <translation>追加されたビュープロパティー「ScaleMultiplier」</translation>
    </message>
    <message>
      <location filename="../../draftobjects/draft_annotation.py" line="125"/>
      <location filename="../../draftobjects/draft_annotation.py" line="130"/>
      <source>migrated 'DraftText' type to 'Text'</source>
      <translation>「DraftText」タイプを「Text」へ変更</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="284"/>
      <source>, path object doesn't have 'Edges'.</source>
      <translation>パスオブジェクトには「エッジ」がありません。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="395"/>
      <location filename="../../draftobjects/patharray.py" line="401"/>
      <location filename="../../draftobjects/patharray.py" line="407"/>
      <source>'PathObj' property will be migrated to 'PathObject'</source>
      <translation>「PathObj」プロパティは「PathObject」に変更されます。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="525"/>
      <source>Cannot calculate path tangent. Copy not aligned.</source>
      <translation>パスの接線を計算できません。コピーは整列されません。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="541"/>
      <source>Tangent and normal are parallel. Copy not aligned.</source>
      <translation>接線と法線が平行です。コピーは整列されません。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="558"/>
      <source>Cannot calculate path normal, using default.</source>
      <translation>デフォルトではパスの法線を計算できません。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="565"/>
      <source>Cannot calculate path binormal. Copy not aligned.</source>
      <translation>パスの従法線を計算できません。コピーは整列されません。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/patharray.py" line="571"/>
      <source>AlignMode {} is not implemented</source>
      <translation>AlignMode {} は実装されていません。</translation>
    </message>
    <message>
      <location filename="../../draftobjects/pointarray.py" line="145"/>
      <location filename="../../draftobjects/pointarray.py" line="161"/>
      <source>added property 'ExtraPlacement'</source>
      <translation>追加されたプロパティー「ExtraPlacement」</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="151"/>
      <source>Object must be a closed shape</source>
      <translation>オブジェクトは閉じた形状でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="153"/>
      <source>No solid object created</source>
      <translation>ソリッドオブジェクトが作成されていません。</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="276"/>
      <source>Faces must be coplanar to be refined</source>
      <translation>高精度化するには面が同一平面上になければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="435"/>
      <location filename="../../draftfunctions/downgrade.py" line="230"/>
      <source>Upgrade: Unknown force method:</source>
      <translation>アップグレード: 不明なメソッドを強制:</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="453"/>
      <source>Found groups: closing each open object inside</source>
      <translation>グループが見つかりました：内部の開いているオブジェクトを閉じます</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="459"/>
      <source>Found meshes: turning into Part shapes</source>
      <translation>メッシュが見つかりました：パート形状へ変換します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="467"/>
      <source>Found 1 solidifiable object: solidifying it</source>
      <translation>ソリッド化可能なオブジェクトが1つ見つかりました：ソリッド化します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="472"/>
      <source>Found 2 objects: fusing them</source>
      <translation>オブジェクトが2つ見つかりました：結合します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="483"/>
      <source>Found object with several coplanar faces: refine them</source>
      <translation>同一平面上にある複数の面を持つオブジェクト見つかりました：高精度化します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="489"/>
      <source>Found 1 non-parametric objects: draftifying it</source>
      <translation>非パラメトリックなオブジェクトが1つ見つかりました：ドラフト化します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="500"/>
      <source>Found 1 closed sketch object: creating a face from it</source>
      <translation>閉じたスケッチオブジェクトが1つ見つかりました：面を作成します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="505"/>
      <source>Found closed wires: creating faces</source>
      <translation>閉じたワイヤーが見つかりました： 面を作成します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="511"/>
      <source>Found several wires or edges: wiring them</source>
      <translation>複数のワイヤー、エッジが見つかりました：ワイヤー化します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="513"/>
      <location filename="../../draftfunctions/upgrade.py" line="547"/>
      <source>Found several non-treatable objects: creating compound</source>
      <translation>複数の修復不能なオブジェクトが見つかりました：コンパウンドを作成します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="518"/>
      <source>trying: closing it</source>
      <translation>試行中：閉じています</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="520"/>
      <source>Found 1 open wire: closing it</source>
      <translation>開いたワイヤーが1つ見つかりました：閉じます</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="537"/>
      <source>Found 1 object: draftifying it</source>
      <translation>オブジェクトが1つ見つかりました：ドラフト化します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="542"/>
      <source>Found points: creating compound</source>
      <translation>点が見つかりました：コンパウドを作成します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/upgrade.py" line="550"/>
      <source>Unable to upgrade these objects.</source>
      <translation>これらのオブジェクトはアップグレードできません。</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="90"/>
      <source>No object given</source>
      <translation>オブジェクトが指定されていません</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="94"/>
      <source>The two points are coincident</source>
      <translation>2点が一致しています</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/mirror.py" line="113"/>
      <source>mirrored</source>
      <translation type="unfinished">mirrored</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="238"/>
      <source>Found 1 block: exploding it</source>
      <translation>ブロックが1つ見つかりました：分解します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="246"/>
      <source>Found 1 multi-solids compound: exploding it</source>
      <translation>コンパウンドが1つ見つかりました：分解します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="253"/>
      <source>Found 1 parametric object: breaking its dependencies</source>
      <translation>パラメトリックなオブジェクトが1つ見つかりました：依存関係を破棄します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="261"/>
      <source>Found 2 objects: subtracting them</source>
      <translation>オブジェクトが2つ見つかりました：減算を行います</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="268"/>
      <source>Found several faces: splitting them</source>
      <translation>複数の面が見つかりました：分割します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="273"/>
      <source>Found several objects: subtracting them from the first one</source>
      <translation>複数のオブジェクトが見つかりました：最初の1つから残りを減算します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="278"/>
      <source>Found 1 face: extracting its wires</source>
      <translation>面が1つ見つかりました：ワイヤーを抽出します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="284"/>
      <source>Found only wires: extracting their edges</source>
      <translation>ワイヤーのみ見つかりました：エッジを抽出します</translation>
    </message>
    <message>
      <location filename="../../draftfunctions/downgrade.py" line="288"/>
      <source>No more downgrade possible</source>
      <translation>これ以上のダウングレードはできません</translation>
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
      <translation>誤った入力: ベクトルでなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_pointarray.py" line="147"/>
      <location filename="../../draftmake/make_text.py" line="107"/>
      <location filename="../../draftmake/make_label.py" line="215"/>
      <source>Wrong input: must be a placement, a vector, or a rotation.</source>
      <translation>誤った入力: 位置、ベクトル、または回転でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="316"/>
      <location filename="../../draftmake/make_label.py" line="230"/>
      <source>Wrong input: object must not be a list.</source>
      <translation>誤った入力: オブジェクトはリストであってはなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_patharray.py" line="213"/>
      <location filename="../../draftmake/make_label.py" line="251"/>
      <source>Wrong input: must be a list or tuple of strings, or a single string.</source>
      <translation>誤った入力: 文字列リスト、文字列タプルまたは文字列でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="263"/>
      <source>Wrong input: subelement not in object.</source>
      <translation>誤った入力: オブジェクトにサブ要素がありません。</translation>
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
      <translation>誤った入力: 文字列リストまたは文字列でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="300"/>
      <location filename="../../draftmake/make_label.py" line="304"/>
      <source>Wrong input: must be a string, 'Horizontal', 'Vertical', or 'Custom'.</source>
      <translation>誤った入力: 「Horizontal」「Vertical」「Custom」のいずれかの文字列でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_polararray.py" line="119"/>
      <location filename="../../draftmake/make_layer.py" line="201"/>
      <location filename="../../draftmake/make_patharray.py" line="191"/>
      <location filename="../../draftmake/make_patharray.py" line="360"/>
      <location filename="../../draftmake/make_orthoarray.py" line="151"/>
      <location filename="../../draftmake/make_label.py" line="313"/>
      <source>Wrong input: must be a number.</source>
      <translation>誤った入力: 数値でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="320"/>
      <source>Wrong input: must be a list of at least two vectors.</source>
      <translation>誤った入力: 少なくとも2つのベクトルのリストでなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="353"/>
      <source>Direction is not 'Custom'; points won't be used.</source>
      <translation>方向が「Custom」ではありません。点は使用されません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_label.py" line="380"/>
      <source>Wrong input: must be a list of two elements. For example, [object, 'Edge1'].</source>
      <translation>誤った入力: 2つの要素のリストでなければなりません。例. [object, 'Edge1']。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_pointarray.py" line="135"/>
      <source>Wrong input: point object doesn't have 'Geometry', 'Links', or 'Components'.</source>
      <translation>誤った入力: 点オブジェクトには「ジオメトリー」「リンク」「コンポーネント」がありません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="69"/>
      <source>Layers</source>
      <translation>レイヤー</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="145"/>
      <location filename="../../draftmake/make_layer.py" line="162"/>
      <location filename="../../draftguitools/gui_layers.py" line="47"/>
      <source>Layer</source>
      <translation>レイヤー</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="157"/>
      <source>Wrong input: it must be a string.</source>
      <translation>誤った入力: 文字列でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="167"/>
      <location filename="../../draftmake/make_layer.py" line="171"/>
      <location filename="../../draftmake/make_layer.py" line="184"/>
      <location filename="../../draftmake/make_layer.py" line="188"/>
      <source>Wrong input: must be a tuple of three floats 0.0 to 1.0.</source>
      <translation>誤った入力: 0.0から1.0の範囲の3つの浮動小数点数のタプルでなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="208"/>
      <location filename="../../draftmake/make_layer.py" line="212"/>
      <source>Wrong input: must be 'Solid', 'Dashed', 'Dotted', or 'Dashdot'.</source>
      <translation>誤った入力: 「Solid」「Dashed」「Dotted」または「Dashdot」でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_layer.py" line="220"/>
      <source>Wrong input: must be a number between 0 and 100.</source>
      <translation>誤った入力: 0から100の範囲の数値でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="173"/>
      <source>This function is deprecated. Do not use this function directly.</source>
      <translation>この機能は非推奨です。この機能を直接使用しないでください。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="174"/>
      <source>Use one of 'make_linear_dimension', or 'make_linear_dimension_obj'.</source>
      <translation>「make_linear_dimension」または「make_linear_dimension_obj」のいずれかを使用してください。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="327"/>
      <location filename="../../draftmake/make_dimension.py" line="452"/>
      <source>Wrong input: object doesn't have a 'Shape' to measure.</source>
      <translation>誤った入力: オブジェクトには測定する「シェイプ」がありません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="331"/>
      <source>Wrong input: object doesn't have at least one element in 'Vertexes' to use for measuring.</source>
      <translation>誤った入力: オブジェクトには測定に使用する「頂点」の要素が1つもありません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="338"/>
      <location filename="../../draftmake/make_dimension.py" line="463"/>
      <source>Wrong input: must be an integer.</source>
      <translation>誤った入力: 整数でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="343"/>
      <source>i1: values below 1 are not allowed; will be set to 1.</source>
      <translation>i1: 1より小さい値は許可されていません。1に設定されます。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="347"/>
      <location filename="../../draftmake/make_dimension.py" line="363"/>
      <source>Wrong input: vertex not in object.</source>
      <translation>誤った入力: オブジェクトに頂点がありません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="359"/>
      <source>i2: values below 1 are not allowed; will be set to the last vertex in the object.</source>
      <translation>i2: 1より小さい値は許可されていません。オブジェクトの最後の頂点に設定されます。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="456"/>
      <source>Wrong input: object doesn't have at least one element in 'Edges' to use for measuring.</source>
      <translation>誤った入力: オブジェクトには測定に使用する「エッジ」の要素が1つもありません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="468"/>
      <source>index: values below 1 are not allowed; will be set to 1.</source>
      <translation>インデックス: 1より小さい値は許可されていません。1に設定されます。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="472"/>
      <source>Wrong input: index doesn't correspond to an edge in the object.</source>
      <translation>誤った入力: インデックスがオブジェクトのエッジに対応していません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="476"/>
      <source>Wrong input: index doesn't correspond to a circular edge.</source>
      <translation>誤った入力: インデックスが円のエッジに対応していません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="483"/>
      <location filename="../../draftmake/make_dimension.py" line="487"/>
      <source>Wrong input: must be a string, 'radius' or 'diameter'.</source>
      <translation>誤った入力: 「radius」または「diameter」の文字列でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_dimension.py" line="579"/>
      <location filename="../../draftmake/make_dimension.py" line="586"/>
      <source>Wrong input: must be a list with two angles.</source>
      <translation>誤った入力: 2つの角度を持つリストでなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="60"/>
      <source>Internal orthogonal array</source>
      <translation>内部矩形配列</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="87"/>
      <source>Wrong input: must be a number or vector.</source>
      <translation>誤った入力: 数値またはベクトルでなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="92"/>
      <location filename="../../draftmake/make_orthoarray.py" line="95"/>
      <location filename="../../draftmake/make_orthoarray.py" line="98"/>
      <source>Input: single value expanded to vector.</source>
      <translation>入力: ベクトルに展開される1つの値。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="154"/>
      <location filename="../../draftmake/make_polararray.py" line="112"/>
      <location filename="../../draftmake/make_orthoarray.py" line="119"/>
      <source>Wrong input: must be an integer number.</source>
      <translation>誤った入力: 整数でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="123"/>
      <location filename="../../draftmake/make_orthoarray.py" line="126"/>
      <location filename="../../draftmake/make_orthoarray.py" line="129"/>
      <source>Input: number of elements must be at least 1. It is set to 1.</source>
      <translation>入力: 要素の数は少なくとも1でなければなりません。1に設定されます。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="275"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="269"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="396"/>
      <source>Orthogonal array</source>
      <translation>矩形整列</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="350"/>
      <source>Orthogonal array 2D</source>
      <translation>矩形整列2D</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="424"/>
      <source>Rectangular array</source>
      <translation>格子状配列</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_orthoarray.py" line="501"/>
      <source>Rectangular array 2D</source>
      <translation>格子状配列2D</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_polararray.py" line="94"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="95"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="258"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="434"/>
      <source>Polar array</source>
      <translation>軸周整列</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_patharray.py" line="246"/>
      <source>Wrong input: must be 'Original', 'Frenet', or 'Tangent'.</source>
      <translation>誤った入力: 「Original」「Frenet」「Tangent」でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="125"/>
      <location filename="../../draftmake/make_arc_3points.py" line="130"/>
      <source>Points:</source>
      <translation type="unfinished">Points:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="126"/>
      <location filename="../../draftmake/make_arc_3points.py" line="131"/>
      <source>Wrong input: must be list or tuple of three points exactly.</source>
      <translation>誤った入力: 3点のリストまたはタプルでなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="138"/>
      <source>Placement:</source>
      <translation type="unfinished">Placement:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="139"/>
      <source>Wrong input: incorrect type of placement.</source>
      <translation>誤った入力: 配置タイプが正しくありません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="153"/>
      <source>Wrong input: incorrect type of points.</source>
      <translation>誤った入力: 点タイプが正しくありません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="159"/>
      <source>Cannot generate shape:</source>
      <translation type="unfinished">Cannot generate shape:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="166"/>
      <source>Radius:</source>
      <translation>半径:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="167"/>
      <source>Center:</source>
      <translation>中心:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="170"/>
      <source>Create primitive object</source>
      <translation>プリミティブオブジェクトを作成</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="193"/>
      <location filename="../../draftmake/make_arc_3points.py" line="204"/>
      <source>Final placement:</source>
      <translation>最終配置:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="195"/>
      <source>Face: True</source>
      <translation>面: True</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="197"/>
      <source>Support:</source>
      <translation>サポート:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_arc_3points.py" line="198"/>
      <source>Map mode:</source>
      <translation>マップモード:</translation>
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
      <translation>円形整列</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_circulararray.py" line="144"/>
      <source>Wrong input: must be a number or quantity.</source>
      <translation>誤った入力: 数字または数量でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="58"/>
      <location filename="../../draftmake/make_fillet.py" line="143"/>
      <location filename="../../draftmake/make_fillet.py" line="144"/>
      <location filename="../../draftmake/make_fillet.py" line="145"/>
      <source>length:</source>
      <translation>長さ:</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="132"/>
      <source>Two elements are needed.</source>
      <translation>2つの要素が必要です。</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="139"/>
      <source>Radius is too large</source>
      <translation>半径が大きすぎます</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="143"/>
      <location filename="../../draftmake/make_fillet.py" line="144"/>
      <location filename="../../draftmake/make_fillet.py" line="145"/>
      <source>Segment</source>
      <translation>セグメント</translation>
    </message>
    <message>
      <location filename="../../draftmake/make_fillet.py" line="165"/>
      <source>Removed original objects.</source>
      <translation>削除された元のオブジェクト。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="87"/>
      <source>Select an object to scale</source>
      <translation>拡大縮小するオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="108"/>
      <source>Pick base point</source>
      <translation>基準点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="135"/>
      <source>Pick reference distance from base point</source>
      <translation>基準点からの参照距離を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="206"/>
      <location filename="../../draftguitools/gui_scale.py" line="236"/>
      <location filename="../../draftguitools/gui_scale.py" line="359"/>
      <source>Scale</source>
      <translation>尺度</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="209"/>
      <source>Some subelements could not be scaled.</source>
      <translation>一部のサブ要素が拡大縮小できませんでした。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="339"/>
      <source>Unable to scale object:</source>
      <translation type="unfinished">Unable to scale object:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="343"/>
      <source>Unable to scale objects:</source>
      <translation type="unfinished">Unable to scale objects:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="346"/>
      <source>This object type cannot be scaled directly. Please use the clone method.</source>
      <translation>このオブジェクトタイプは直接は拡大縮小できません。クローンメソッドを使用してください。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_scale.py" line="407"/>
      <source>Pick new distance from base point</source>
      <translation>基準点からの新しい距離を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_sketcher_objects.py" line="63"/>
      <source>Sketch is too complex to edit: it is suggested to use sketcher default editor</source>
      <translation>スケッチが複雑すぎて編集できません: スケッチャーのデフォルトエディターを使用することをお勧めします。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="80"/>
      <source>Pick target point</source>
      <translation>対象点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="157"/>
      <source>Create Label</source>
      <translation>ラベルを作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="191"/>
      <location filename="../../draftguitools/gui_labels.py" line="218"/>
      <source>Pick endpoint of leader line</source>
      <translation>引き出し線の終点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_labels.py" line="201"/>
      <location filename="../../draftguitools/gui_labels.py" line="228"/>
      <source>Pick text position</source>
      <translation>テキスト位置を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_styles.py" line="75"/>
      <source>Change Style</source>
      <translation>スタイルを変更</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="77"/>
      <source>Pick location point</source>
      <translation>配置点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_texts.py" line="121"/>
      <source>Create Text</source>
      <translation>テキストを作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_grid.py" line="51"/>
      <source>Toggle grid</source>
      <translation>グリッドを切り替え</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="150"/>
      <source>Pick a face, 3 vertices or a WP Proxy to define the drawing plane</source>
      <translation>描画平面を定義する面、3つの頂点、またはWPプロキシを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="274"/>
      <source>Working plane aligned to global placement of</source>
      <translation>作業平面は以下のグローバル位置に配置されました：</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="523"/>
      <source>Dir</source>
      <translation>Dir</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_selectplane.py" line="539"/>
      <source>Custom</source>
      <translation>カスタム設定</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_facebinders.py" line="71"/>
      <source>Select faces from existing objects</source>
      <translation>既存のオブジェクトの面を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="73"/>
      <source>Select an object to mirror</source>
      <translation>鏡像コピーするオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="92"/>
      <source>Pick start point of mirror line</source>
      <translation>対称線の始点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="122"/>
      <source>Mirror</source>
      <translation>鏡像コピー</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_mirror.py" line="177"/>
      <location filename="../../draftguitools/gui_mirror.py" line="203"/>
      <source>Pick end point of mirror line</source>
      <translation>対称線の終点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="78"/>
      <location filename="../../draftguitools/gui_arcs.py" line="88"/>
      <source>Pick center point</source>
      <translation>中心点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="189"/>
      <location filename="../../draftguitools/gui_polygons.py" line="200"/>
      <location filename="../../draftguitools/gui_polygons.py" line="260"/>
      <location filename="../../draftguitools/gui_arcs.py" line="254"/>
      <location filename="../../draftguitools/gui_arcs.py" line="270"/>
      <location filename="../../draftguitools/gui_arcs.py" line="410"/>
      <source>Pick radius</source>
      <translation>半径を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="277"/>
      <location filename="../../draftguitools/gui_arcs.py" line="278"/>
      <location filename="../../draftguitools/gui_arcs.py" line="446"/>
      <location filename="../../draftguitools/gui_arcs.py" line="447"/>
      <source>Start angle</source>
      <translation>開始角度</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="283"/>
      <location filename="../../draftguitools/gui_arcs.py" line="452"/>
      <source>Pick start angle</source>
      <translation>開始角度を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="285"/>
      <location filename="../../draftguitools/gui_arcs.py" line="286"/>
      <location filename="../../draftguitools/gui_arcs.py" line="454"/>
      <location filename="../../draftguitools/gui_arcs.py" line="455"/>
      <source>Aperture angle</source>
      <translation>開口角</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="291"/>
      <source>Pick aperture</source>
      <translation>開口を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="317"/>
      <source>Create Circle (Part)</source>
      <translation>円を作成 (パート)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="335"/>
      <source>Create Circle</source>
      <translation>円を作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="369"/>
      <source>Create Arc (Part)</source>
      <translation>円弧を作成 (パート)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="389"/>
      <source>Create Arc</source>
      <translation>円弧を作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="466"/>
      <source>Pick aperture angle</source>
      <translation>開口角を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_arcs.py" line="509"/>
      <location filename="../../draftguitools/gui_arcs.py" line="551"/>
      <source>Arc by 3 points</source>
      <translation>3点から決まる円弧</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="71"/>
      <location filename="../../draftguitools/gui_lines.py" line="83"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="122"/>
      <location filename="../../draftguitools/gui_rectangles.py" line="69"/>
      <source>Pick first point</source>
      <translation>最初の点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="163"/>
      <source>Create Line</source>
      <translation>直線を作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="185"/>
      <source>Create Wire</source>
      <translation>ワイヤーを作成</translation>
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
      <translation>次の点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="330"/>
      <source>Unable to create a Wire from selected objects</source>
      <translation>選択したオブジェクトからワイヤーを作成できません。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lines.py" line="352"/>
      <source>Convert to Wire</source>
      <translation>ワイヤーに変換</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shapestrings.py" line="85"/>
      <location filename="../../draftguitools/gui_shapestrings.py" line="98"/>
      <source>Pick ShapeString location point</source>
      <translation>シェイプストリングの配置点を選択</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="176"/>
      <location filename="../../draftguitools/gui_shapestrings.py" line="133"/>
      <source>Create ShapeString</source>
      <translation>シェイプストリングを作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="305"/>
      <source>Select a Draft object to edit</source>
      <translation>編集するDraftオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="558"/>
      <source>No edit point found for selected object</source>
      <translation>選択オブジェクトの中に編集点がありません。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="811"/>
      <source>Too many objects selected, max number set to:</source>
      <translation type="unfinished">Too many objects selected, max number set to:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit.py" line="819"/>
      <source>: this object is not editable</source>
      <translation>: このオブジェクトは編集できません</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="76"/>
      <source>Select an object to join</source>
      <translation>結合するオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="99"/>
      <source>Join lines</source>
      <translation>線の結合</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_join.py" line="110"/>
      <source>Selection:</source>
      <translation>選択:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_lineslope.py" line="64"/>
      <source>Change slope</source>
      <translation>傾斜を変更</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="94"/>
      <source>Select objects to trim or extend</source>
      <translation>トリムまたは伸縮するオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="173"/>
      <location filename="../../draftguitools/gui_offset.py" line="143"/>
      <source>Pick distance</source>
      <translation>距離を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="222"/>
      <source>Offset angle</source>
      <translation type="unfinished">Offset angle</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="483"/>
      <source>Unable to trim these objects, only Draft wires and arcs are supported.</source>
      <translation>これらのオブジェクトをトリムすることはできません。サポートされているのはドラフトのワイヤーと円弧のみです。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="488"/>
      <source>Unable to trim these objects, too many wires</source>
      <translation>これらのオブジェクトはワイヤーが多すぎてトリムすることができません。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="505"/>
      <source>These objects don't intersect.</source>
      <translation>これらのオブジェクトは交差してません。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_trimex.py" line="508"/>
      <source>Too many intersection points.</source>
      <translation>交点が多すぎます。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="66"/>
      <source>Select an object to convert.</source>
      <translation>変換するオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="99"/>
      <source>Convert to Sketch</source>
      <translation>スケッチに変換</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="114"/>
      <source>Convert to Draft</source>
      <translation>ドラフトに変換</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_draft2sketch.py" line="143"/>
      <source>Convert Draft/Sketch</source>
      <translation>ドラフト/スケッチを変換</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="104"/>
      <source>Please select exactly two objects, the base object and the point object, before calling this command.</source>
      <translation>このコマンドを呼び出す前に、ベース・オブジェクトとポイント・オブジェクトの2つのオブジェクトを選択してください。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pointarray.py" line="122"/>
      <source>Point array</source>
      <translation>点整列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_subelements.py" line="108"/>
      <source>Select an object to edit</source>
      <translation>編集するオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_clone.py" line="79"/>
      <source>Select an object to clone</source>
      <translation>クローンするオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="212"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="247"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="261"/>
      <location filename="../../draftguitools/gui_dimensions.py" line="294"/>
      <source>Create Dimension</source>
      <translation>寸法を作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="309"/>
      <source>Create Dimension (radial)</source>
      <translation>寸法（半径）を作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="508"/>
      <source>Edge too short!</source>
      <translation type="unfinished">Edge too short!</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimensions.py" line="518"/>
      <source>Edges don't intersect!</source>
      <translation>エッジが交差していません！</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="75"/>
      <source>Select an object to stretch</source>
      <translation>伸縮するオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="127"/>
      <source>Pick first point of selection rectangle</source>
      <translation>選択四角形の最初の点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="164"/>
      <source>Pick opposite point of selection rectangle</source>
      <translation>選択四角形の対角点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="173"/>
      <source>Pick start point of displacement</source>
      <translation>移動の始点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="236"/>
      <source>Pick end point of displacement</source>
      <translation>移動の終点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="448"/>
      <source>Turning one Rectangle into a Wire</source>
      <translation>1つの四角形をワイヤーへ変換</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_stretch.py" line="477"/>
      <source>Stretch</source>
      <translation>伸縮</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="102"/>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="79"/>
      <source>Please select exactly two objects, the base object and the path object, before calling this command.</source>
      <translation>このコマンドを呼び出す前に、ベース・オブジェクトとパス・オブジェクトの2つのオブジェクトを選択してください。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_pathtwistedarray.py" line="101"/>
      <source>Path twisted array</source>
      <translation>パスツイスト整列</translation>
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
      <translation>最後の点が取り除かれました。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="153"/>
      <location filename="../../draftguitools/gui_splines.py" line="147"/>
      <source>Pick next point, or finish (A) or close (O)</source>
      <translation>次の点を選択、または終了 (A)、閉じる (O)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="211"/>
      <location filename="../../draftguitools/gui_beziers.py" line="451"/>
      <source>Create BezCurve</source>
      <translation>ベジェ曲線を作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="376"/>
      <source>Click and drag to define next knot</source>
      <translation>次のノットを定義するためにクリックしてドラッグ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_beziers.py" line="382"/>
      <source>Click and drag to define next knot, or finish (A) or close (O)</source>
      <translation>次のノットを定義するためにクリックしてドラッグ、または終了(A) 、閉じる(O)</translation>
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
      <translation>アップグレードするオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_downgrade.py" line="85"/>
      <source>Downgrade</source>
      <translation>ダウングレード</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_patharray.py" line="143"/>
      <source>Path array</source>
      <translation>パス整列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="120"/>
      <source>Spline has been closed</source>
      <translation>スプラインが閉じられました。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_splines.py" line="183"/>
      <source>Create B-spline</source>
      <translation>B-スプラインを作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="125"/>
      <source>Create Plane</source>
      <translation>平面を作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rectangles.py" line="142"/>
      <source>Create Rectangle</source>
      <translation>長方形を作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="196"/>
      <location filename="../../draftguitools/gui_rectangles.py" line="202"/>
      <source>Pick opposite point</source>
      <translation>反対側の点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="76"/>
      <source>Fillet radius</source>
      <translation>フィレット半径</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="79"/>
      <source>Radius of fillet</source>
      <translation>フィレットの半径</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="107"/>
      <source>Enter radius.</source>
      <translation>半径を入力してください。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="126"/>
      <source>Delete original objects:</source>
      <translation>元のオブジェクトを削除:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="131"/>
      <source>Chamfer mode:</source>
      <translation>面取りモード:</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="148"/>
      <source>Two elements needed.</source>
      <translation>2つの要素が必要です。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="155"/>
      <source>Test object</source>
      <translation>テスト・オブジェクト</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="156"/>
      <source>Test object removed</source>
      <translation>テスト・オブジェクトが削除されました。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="158"/>
      <source>Fillet cannot be created</source>
      <translation>フィレットを作成することができません</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_fillets.py" line="188"/>
      <source>Create fillet</source>
      <translation>フィレットを作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="65"/>
      <source>Add to group</source>
      <translation>グループに追加</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="68"/>
      <source>Ungroup</source>
      <translation>グループ解除</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="70"/>
      <source>Add new group</source>
      <translation type="unfinished">Add new group</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="159"/>
      <source>Select group</source>
      <translation>グループを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="193"/>
      <source>No new selection. You must select non-empty groups or objects inside groups.</source>
      <translation type="unfinished">No new selection. You must select non-empty groups or objects inside groups.</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="203"/>
      <source>Autogroup</source>
      <translation>オートグループ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="250"/>
      <source>Add new Layer</source>
      <translation>新しいレイヤーを追加</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="304"/>
      <source>Add to construction group</source>
      <translation>構築グループに追加</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="355"/>
      <source>Add a new group with a given name</source>
      <translation type="unfinished">Add a new group with a given name</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="383"/>
      <source>Add group</source>
      <translation type="unfinished">Add group</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="385"/>
      <source>Group name</source>
      <translation type="unfinished">Group name</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_groups.py" line="392"/>
      <source>Group</source>
      <translation>グループ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="92"/>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="569"/>
      <source>This object does not support possible coincident points, please try again.</source>
      <translation>このオブジェクトは一致点をサポートしていません。もう一度やり直してください。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="164"/>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="608"/>
      <source>Active object must have more than two points/nodes</source>
      <translation>アクティブなオブジェクトには２つ以上の点/節点が必要です。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="722"/>
      <source>Selection is not a Knot</source>
      <translation>ノットが選択されていません。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_edit_draft_objects.py" line="749"/>
      <source>Endpoint of BezCurve can't be smoothed</source>
      <translation>ベジエ曲線の端点はスムージングできません。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="77"/>
      <source>The Drawing Workbench is obsolete since 0.17, consider using the TechDraw Workbench instead.</source>
      <translation>Drawingワークベンチは0.17以降廃止されています。代わりにTechDrawワークベンチを使用することを検討してください。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_drawing.py" line="81"/>
      <location filename="../../draftguitools/gui_shape2dview.py" line="68"/>
      <source>Select an object to project</source>
      <translation>投影するオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_upgrade.py" line="85"/>
      <source>Upgrade</source>
      <translation>アップグレード</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="126"/>
      <source>Main toggle snap</source>
      <translation>メイン切り替えスナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="157"/>
      <source>Midpoint snap</source>
      <translation>中点スナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="187"/>
      <source>Perpendicular snap</source>
      <translation>垂直スナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="217"/>
      <source>Grid snap</source>
      <translation>グリッドにスナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="247"/>
      <source>Intersection snap</source>
      <translation>交点スナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="277"/>
      <source>Parallel snap</source>
      <translation>平行スナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="307"/>
      <source>Endpoint snap</source>
      <translation>端点スナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="338"/>
      <source>Angle snap (30 and 45 degrees)</source>
      <translation>角度スナップ (30度、45度)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="368"/>
      <source>Arc center snap</source>
      <translation>円弧中心スナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="398"/>
      <source>Edge extension snap</source>
      <translation>エッジ延長スナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="428"/>
      <source>Near snap</source>
      <translation>近接スナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="459"/>
      <source>Orthogonal snap</source>
      <translation>直交スナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="489"/>
      <source>Special point snap</source>
      <translation>特殊点スナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="520"/>
      <source>Dimension display</source>
      <translation>寸法表示</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="553"/>
      <source>Working plane snap</source>
      <translation>作業平面スナップ</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_snaps.py" line="583"/>
      <source>Show snap toolbar</source>
      <translation>スナップ用ツールバーを表示</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="81"/>
      <source>Select an object to move</source>
      <translation>移動するオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="103"/>
      <source>Pick start point</source>
      <translation>始点を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_move.py" line="162"/>
      <location filename="../../draftguitools/gui_move.py" line="308"/>
      <source>Pick end point</source>
      <translation>終点を選択</translation>
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
      <translation>一部のサブ要素を移動できませんでした。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_ellipses.py" line="121"/>
      <location filename="../../draftguitools/gui_ellipses.py" line="138"/>
      <source>Create Ellipse</source>
      <translation>楕円を作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_dimension_ops.py" line="55"/>
      <source>Flip dimension</source>
      <translation>寸法を反転</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="73"/>
      <source>No active Draft Toolbar.</source>
      <translation>アクティブなドラフト・ツールバーがありません。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="93"/>
      <source>Construction mode</source>
      <translation>構築モード</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="125"/>
      <source>Continue mode</source>
      <translation>継続モード</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_togglemodes.py" line="159"/>
      <source>Toggle display mode</source>
      <translation>表示モードを切り替え</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="68"/>
      <source>Annotation style editor</source>
      <translation>注釈スタイルエディター</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="290"/>
      <source>Open styles file</source>
      <translation>スタイルファイルを開く</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="292"/>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="314"/>
      <source>JSON file (*.json)</source>
      <translation>JSONファイル (*.json)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_annotationstyleeditor.py" line="312"/>
      <source>Save styles file</source>
      <translation>スタイルファイルを保存</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_heal.py" line="51"/>
      <source>Heal</source>
      <translation>修復</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_points.py" line="134"/>
      <location filename="../../draftguitools/gui_points.py" line="147"/>
      <source>Create Point</source>
      <translation>点を作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="224"/>
      <source>Create Polygon (Part)</source>
      <translation>多角形を作成 (パート)</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_polygons.py" line="243"/>
      <source>Create Polygon</source>
      <translation>多角形を作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="76"/>
      <source>Select an object to offset</source>
      <translation>オフセットするオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="82"/>
      <source>Offset only works on one object at a time.</source>
      <translation>一度にオフセットできるのは1つのオブジェクトだけです。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="92"/>
      <source>Cannot offset this object type</source>
      <translation>この種類のオブジェクトはオフセットできません。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_offset.py" line="123"/>
      <source>Offset of Bezier curves is currently not supported</source>
      <translation>ベジェ曲線のオフセットは現在サポートされていません</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="79"/>
      <source>Select an object to rotate</source>
      <translation>回転するオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="99"/>
      <source>Pick rotation center</source>
      <translation>回転中心を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="193"/>
      <location filename="../../draftguitools/gui_rotate.py" line="396"/>
      <source>Base angle</source>
      <translation>ベース角度</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="194"/>
      <location filename="../../draftguitools/gui_rotate.py" line="397"/>
      <source>The base angle you wish to start the rotation from</source>
      <translation>回転を開始するベース角度</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="199"/>
      <location filename="../../draftguitools/gui_rotate.py" line="400"/>
      <source>Pick base angle</source>
      <translation>ベース角度を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="205"/>
      <location filename="../../draftguitools/gui_rotate.py" line="409"/>
      <source>Rotation</source>
      <translation>回転</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="206"/>
      <location filename="../../draftguitools/gui_rotate.py" line="410"/>
      <source>The amount of rotation you wish to perform.
The final angle will be the base angle plus this amount.</source>
      <translation>実行したい回転の量。
最終角度はベース角度とこの量の合計になります。</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_rotate.py" line="213"/>
      <location filename="../../draftguitools/gui_rotate.py" line="418"/>
      <source>Pick rotation angle</source>
      <translation>回転角度を選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_shape2dview.py" line="109"/>
      <source>Create 2D view</source>
      <translation>2Dビューを作成</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="77"/>
      <source>Select an object to array</source>
      <translation>整列させるオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_array_simple.py" line="101"/>
      <source>Array</source>
      <translation>配列</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="64"/>
      <source>Click anywhere on a line to split it.</source>
      <translation>分割するために線上の任意の場所をクリック</translation>
    </message>
    <message>
      <location filename="../../draftguitools/gui_split.py" line="106"/>
      <source>Split line</source>
      <translation>線を分割</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="83"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="83"/>
      <source>Task panel:</source>
      <translation>タスクパネル:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="187"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="208"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="201"/>
      <source>At least one element must be selected.</source>
      <translation>少なくとも1つの要素を選択する必要があります。</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="205"/>
      <source>Number of elements must be at least 1.</source>
      <translation>要素の数は少なくとも1でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="194"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="219"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="212"/>
      <source>Selection is not suitable for array.</source>
      <translation>選択したものは整列に適していません。</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="195"/>
      <location filename="../../drafttaskpanels/task_polararray.py" line="327"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="220"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="372"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="213"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="375"/>
      <source>Object:</source>
      <translation>オブジェクト:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="316"/>
      <source>Interval X reset:</source>
      <translation>間隔 X リセット:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="325"/>
      <source>Interval Y reset:</source>
      <translation>間隔 Y リセット:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="334"/>
      <source>Interval Z reset:</source>
      <translation>間隔 Z リセット:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="296"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="341"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="344"/>
      <source>Fuse:</source>
      <translation>結合:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="310"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="355"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="358"/>
      <source>Create Link array:</source>
      <translation>リンク整列を作成:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="376"/>
      <source>Number of X elements:</source>
      <translation>X 要素の数:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="378"/>
      <source>Interval X:</source>
      <translation>間隔 X:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="381"/>
      <source>Number of Y elements:</source>
      <translation>Y 要素の数:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="383"/>
      <source>Interval Y:</source>
      <translation>間隔 Y:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="386"/>
      <source>Number of Z elements:</source>
      <translation>Z 要素の数:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="388"/>
      <source>Interval Z:</source>
      <translation>間隔 Z:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="434"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="481"/>
      <location filename="../../drafttaskpanels/task_orthoarray.py" line="396"/>
      <source>Aborted:</source>
      <translation>中断:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="212"/>
      <source>Number of layers must be at least 2.</source>
      <translation>レイヤーの数は少なくとも2でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="224"/>
      <source>Radial distance is zero. Resulting array may not look correct.</source>
      <translation>周方向距離がゼロです。整列結果が正しく見えない可能性があります。</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="226"/>
      <source>Radial distance is negative. It is made positive to proceed.</source>
      <translation>半径方向距離がマイナスです。処理を続けるためプラスにします。</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="230"/>
      <source>Tangential distance cannot be zero.</source>
      <translation>接線方向距離はゼロにはできません。</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="233"/>
      <source>Tangential distance is negative. It is made positive to proceed.</source>
      <translation>接線方向距離がマイナスです。処理を続けるためプラスにします。</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="286"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="331"/>
      <source>Center reset:</source>
      <translation>中心リセット:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="373"/>
      <source>Radial distance:</source>
      <translation>半径距離:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="374"/>
      <source>Tangential distance:</source>
      <translation>接線距離:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="375"/>
      <source>Number of circular layers:</source>
      <translation>円形レイヤーの数:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="376"/>
      <source>Symmetry parameter:</source>
      <translation>対称パラメーター:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="331"/>
      <location filename="../../drafttaskpanels/task_circulararray.py" line="378"/>
      <source>Center of rotation:</source>
      <translation>回転中心:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="199"/>
      <source>Number of elements must be at least 2.</source>
      <translation>要素の数は少なくとも2でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="203"/>
      <source>The angle is above 360 degrees. It is set to this value to proceed.</source>
      <translation>角度が360度を超えています。続行するとこの値に設定されます。</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="206"/>
      <source>The angle is below -360 degrees. It is set to this value to proceed.</source>
      <translation>角度が-360度を下回っています。続行するとこの値に設定されます。</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="328"/>
      <source>Number of elements:</source>
      <translation>要素の数:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_polararray.py" line="329"/>
      <source>Polar angle:</source>
      <translation>軸周角度:</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="56"/>
      <source>ShapeString</source>
      <translation>シェイプストリング</translation>
    </message>
    <message>
      <location filename="../../drafttaskpanels/task_shapestring.py" line="70"/>
      <source>Default</source>
      <translation>デフォルト</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="361"/>
      <source>Activate this layer</source>
      <translation>このレイヤーをアクティブにする</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="367"/>
      <source>Select layer contents</source>
      <translation>レイヤーの内容を選択</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="405"/>
      <location filename="../../draftviewproviders/view_layer.py" line="421"/>
      <source>Merge layer duplicates</source>
      <translation>レイヤーの重複を結合</translation>
    </message>
    <message>
      <location filename="../../draftviewproviders/view_layer.py" line="410"/>
      <location filename="../../draftviewproviders/view_layer.py" line="469"/>
      <source>Add new layer</source>
      <translation>新しいレイヤーの追加</translation>
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
      <translation>OCA エラー: 文字エンコーディングが判別できませんでした</translation>
    </message>
    <message>
      <location filename="../../importOCA.py" line="445"/>
      <source>OCA: found no data to export</source>
      <translation>OCA: エクスポートするデータが見つかりませんでした</translation>
    </message>
    <message>
      <location filename="../../importOCA.py" line="490"/>
      <source>successfully exported</source>
      <translation type="unfinished">successfully exported</translation>
    </message>
  </context>
</TS>
