<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ja" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="80"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>参照軸から決定されるらせん開始位置の中心点</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="90"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>参照軸から決定されるらせん方向</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="97"/>
      <source>The reference axis of the helix.</source>
      <translation>らせんの参照軸</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="104"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>らせん入力モードでは、どのプロパティをユーザー設定とするかを指定します。
この時、依存プロパティが計算されます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="116"/>
      <source>The axial distance between two turns.</source>
      <translation>2つの巻き間の軸距離</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="123"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>プロファイル範囲に基づかないらせんパスの高さ</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="133"/>
      <source>The number of turns in the helix.</source>
      <translation>らせんの巻き数</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="141"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, negative shrinks.</source>
      <translation>らせんの周りに外殻を形成する円錐の角度。
0 以外の値を設定すると、らせんは円錐形になります。
正の値では半径が大きくなり、負の値では半径が小さくなります。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="154"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>1巻きあたりのらせん半径の増加率。
ゼロ以外の値では、らせんが円錐形らせんになります。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="165"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>回転方向を左手系に設定。
つまり軸に沿って移動する時に反時計回りになります。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="176"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>らせんの点を軸反対方向に配置するかどうかを指定</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="186"/>
      <source>If set, the result will be the intersection of the profile and the preexisting body.</source>
      <translation>設定されている場合、プロファイルと既存ボディーの交点となります。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="196"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>False の場合、ツールは自己交差を回避するようにプロファイルのバウンディングボックスに基づいてピッチの初期値を提案します。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="208"/>
      <source>Fusion Tolerance for the Helix, increase if helical shape does not merge nicely with part.</source>
      <translation>らせんの結合トレランス。らせん形状がパーツとうまく結合しない場合には増やしてください。</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="106"/>
      <source>Number of gear teeth</source>
      <translation>歯数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="118"/>
      <source>Pressure angle of gear teeth</source>
      <translation>歯の圧力角</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="112"/>
      <source>Module of the gear</source>
      <translation>歯車のモジュール</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="127"/>
      <source>True=2 curves with each 3 control points, False=1 curve with 4 control points.</source>
      <translation>True=それぞれ3つの制御点を持つ2曲線、False=4つの制御点を持つ1曲線</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="135"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation>True=外歯車、False=内歯車</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="144"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>モジュールによって正規化された、ピッチ円から歯先までの歯の高さ</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="153"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>モジュールによって正規化された、ピッチ円から歯元までの歯の高さ</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="162"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>モジュールによって正規化された、歯の根元のフィレットの半径</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="171"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>モジュールによって正規化された、参照プロファイルを外方向にシフトする距離</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1660"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1661"/>
      <source>Additive Helix</source>
      <translation>加算らせん</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1662"/>
      <source>Sweeps the selected sketch or profile along a helix and adds it to the body</source>
      <translation>選択したスケッチまたはプロファイルをらせんに沿ってスイープし、ボディーに追加</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1561"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1562"/>
      <source>Additive Loft</source>
      <translation>加算ロフト</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1563"/>
      <source>Lofts the selected sketch or profile along a path and adds it to the body</source>
      <translation>選択したスケッチまたはプロファイルを経路に沿ってロフトし、ボディーに追加</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1461"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1462"/>
      <source>Additive Pipe</source>
      <translation>加算パイプ</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1463"/>
      <source>Sweeps the selected sketch or profile along a path and adds it to the body</source>
      <translation>選択したスケッチまたはプロファイルを経路に沿ってスイープし、ボディーに追加</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="90"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="91"/>
      <source>New Body</source>
      <translation>新しいボディー</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="92"/>
      <source>Creates a new body and activates it</source>
      <translation>新しいボディーを作成し、アクティブ化</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2576"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2577"/>
      <source>Boolean Operation</source>
      <translation>ブーリアン演算</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2578"/>
      <source>Applies boolean operations with the selected objects and the active body</source>
      <translation>選択したオブジェクトとアクティブなボディーを使ってブール演算を適用</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="279"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="280"/>
      <source>Local Coordinate System</source>
      <translation>ローカル座標系</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="281"/>
      <source>Creates a new local coordinate system</source>
      <translation>新しいローカル座標系を作成</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="1987"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1988"/>
      <source>Chamfer</source>
      <translation>面取り</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1989"/>
      <source>Applies a chamfer to the selected edges or faces</source>
      <translation>選択したエッジまたは面に面取りを適用</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="489"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="490"/>
      <source>Clone</source>
      <translation>クローン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="491"/>
      <source>Copies a solid object parametrically as the base feature of a new body</source>
      <translation>ソリッドオブジェクトを新しいボディーのベースフィーチャーとしてパラメトリックにコピー</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="2016"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2017"/>
      <source>Draft</source>
      <translation>抜き勾配</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2018"/>
      <source>Applies a draft to the selected faces</source>
      <translation>選択した面に抜き勾配を適用</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="755"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="756"/>
      <source>Duplicate &amp;Object</source>
      <translation>オブジェクトを複製(&amp;O)</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="757"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>選択したオブジェクトを複製し、アクティブなボディーに追加</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1959"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1960"/>
      <source>Fillet</source>
      <translation>フィレット</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1961"/>
      <source>Applies a fillet to the selected edges or faces</source>
      <translation>選択したエッジまたは面にフィレットを適用</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1391"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1392"/>
      <source>Groove</source>
      <translation>グルーブ</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1393"/>
      <source>Revolves the sketch or profile around a line or axis and removes it from the body</source>
      <translation>スケッチまたはプロファイルを、線または軸の周りに回転押し出しし、ボディーから削除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1284"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1285"/>
      <source>Hole</source>
      <translation>ホール</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1287"/>
      <source>Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</source>
      <translation>アクティブなボディーで、選択したスケッチまたはプロファイルにある、円または円弧の中心点に穴を作成</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="219"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="220"/>
      <source>Datum Line</source>
      <translation>データム線</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="221"/>
      <source>Creates a new datum line</source>
      <translation>新しいデータム線を作成</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2271"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2272"/>
      <source>Linear Pattern</source>
      <translation>直線状パターン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2273"/>
      <source>Duplicates the selected features or the active body in a linear pattern</source>
      <translation>選択したフィーチャーまたはアクティブなボディーを直線状パターンで複製</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="385"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="386"/>
      <source>Migrate</source>
      <translation>移行</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="387"/>
      <source>Migrates the document to the modern Part Design workflow</source>
      <translation>新しいパートデザイン・ワークフローへドキュメントを移行</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="2214"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2215"/>
      <source>Mirror</source>
      <translation>鏡像</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2216"/>
      <source>Mirrors the selected features or active body</source>
      <translation>選択したフィーチャーまたはアクティブなボディーを鏡像コピー</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="821"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="822"/>
      <source>Move Object To…</source>
      <translation>オブジェクトを移動…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="823"/>
      <source>Moves the selected object to another body</source>
      <translation>選択したオブジェクトを他のボディーへ移動</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1016"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1017"/>
      <source>Move Feature After…</source>
      <translation>フィーチャーを次の後ろへ移動…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1018"/>
      <source>Moves the selected feature after another feature in the same body</source>
      <translation>選択したフィーチャーを同じボディーの別のフィーチャーの後に移動</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="658"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="659"/>
      <source>Set Tip</source>
      <translation>TIPを設定</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="660"/>
      <source>Moves the tip of the body to the selected feature</source>
      <translation>選択したフィーチャーにボディーのTIPを移動</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2445"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2446"/>
      <source>Multi-Transform</source>
      <translation>マルチ変換</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2447"/>
      <source>Applies multiple transformations to the selected features or active body</source>
      <translation>選択したフィーチャーまたはアクティブなボディーに複数の変換を適用</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="573"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="574"/>
      <source>New Sketch</source>
      <translation>新しいスケッチ</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="575"/>
      <source>Creates a new sketch</source>
      <translation>新しいスケッチを作成</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1226"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1227"/>
      <source>Pad</source>
      <translation>パッド</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1228"/>
      <source>Extrudes the selected sketch or profile and adds it to the body</source>
      <translation>選択したスケッチまたはプロファイルを押し出し、ボディーに追加</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="189"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="190"/>
      <source>Datum Plane</source>
      <translation>データム平面</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="191"/>
      <source>Creates a new datum plane</source>
      <translation>新しいデータム平面を作成</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1255"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1256"/>
      <source>Pocket</source>
      <translation>ポケット</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1257"/>
      <source>Extrudes the selected sketch or profile and removes it from the body</source>
      <translation>選択したスケッチまたはプロファイルを押し出し、ボディーから削除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="249"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="250"/>
      <source>Datum Point</source>
      <translation>データム点</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="251"/>
      <source>Creates a new datum point</source>
      <translation>新しいデータム点を作成</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2340"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2341"/>
      <source>Polar Pattern</source>
      <translation>軸周状パターン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2342"/>
      <source>Duplicates the selected features or the active body in a circular pattern</source>
      <translation>選択したフィーチャーまたはアクティブなボディーを円形パターンで複製</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1329"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1330"/>
      <source>Revolve</source>
      <translation>回転</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1331"/>
      <source>Revolves the selected sketch or profile around a line or axis and adds it to the body</source>
      <translation>選択したスケッチまたはプロファイルを、線または軸の周りに回転押し出しし、ボディーに追加</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2402"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2403"/>
      <source>Scale</source>
      <translation>尺度</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2404"/>
      <source>Scales the selected features or the active body</source>
      <translation>選択したフィーチャーまたはアクティブなボディーを拡大縮小</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="313"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="314"/>
      <source>Shape Binder</source>
      <translation>シェイプバインダー</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="315"/>
      <source>Creates a new shape binder</source>
      <translation>新しいシェイプバインダーを作成</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="383"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="384"/>
      <source>Sub-Shape Binder</source>
      <translation>サブシェイプバインダー</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="385"/>
      <source>Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</source>
      <translation>1つまたは複数のオブジェクトからボディーの内外で使用できるジオメトリーへの参照を作成します。この参照は相対配置を維持し、複数のジオメトリータイプ（ソリッド、面、エッジ、頂点）をサポートし、同じドキュメントまたは外部ドキュメント内のオブジェクトと共に使用できます。</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1744"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1745"/>
      <source>Subtractive Helix</source>
      <translation>減算らせん</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1746"/>
      <source>Sweeps the selected sketch or profile along a helix and removes it from the body</source>
      <translation>選択したスケッチまたはプロファイルをらせんに沿ってスイープし、ボディーから削除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1611"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1612"/>
      <source>Subtractive Loft</source>
      <translation>減算ロフト</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1613"/>
      <source>Lofts the selected sketch or profile along a path and removes it from the body</source>
      <translation>選択したスケッチまたはプロファイルを経路に沿ってロフトし、ボディーから削除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1511"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1512"/>
      <source>Subtractive Pipe</source>
      <translation>減算パイプ</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1513"/>
      <source>Sweeps the selected sketch or profile along a path and removes it from the body</source>
      <translation>選択したスケッチまたはプロファイルを経路に沿ってスイープし、ボディーから削除</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="2086"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2087"/>
      <source>Thickness</source>
      <translation>厚み</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2088"/>
      <source>Applies thickness and removes the selected faces</source>
      <translation>厚みを適用し、選択した面を削除</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="74"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="75"/>
      <source>Additive Primitive</source>
      <translation>加算プリミティブ</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="76"/>
      <source>Creates an additive primitive</source>
      <translation>加算プリミティブを作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="213"/>
      <source>Additive Box</source>
      <translation>加算直方体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="222"/>
      <source>Additive Cylinder</source>
      <translation>加算円柱</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="231"/>
      <source>Additive Sphere</source>
      <translation>加算球</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="240"/>
      <source>Additive Cone</source>
      <translation>加算円錐</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="246"/>
      <source>Additive Ellipsoid</source>
      <translation>加算楕円体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="252"/>
      <source>Additive Torus</source>
      <translation>加算トーラス</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="258"/>
      <source>Additive Prism</source>
      <translation>加算角柱</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="264"/>
      <source>Additive Wedge</source>
      <translation>加算ウェッジ</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="282"/>
      <source>PartDesign</source>
      <translation>パートデザイン</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="283"/>
      <source>Subtractive Primitive</source>
      <translation>減算プリミティブ</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="284"/>
      <source>Creates a subtractive primitive</source>
      <translation>減算プリミティブを作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="398"/>
      <source>Subtractive Box</source>
      <translation>減算直方体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="407"/>
      <source>Subtractive Cylinder</source>
      <translation>減算円柱</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="416"/>
      <source>Subtractive Sphere</source>
      <translation>減算球</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="425"/>
      <source>Subtractive Cone</source>
      <translation>減算円錐</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="431"/>
      <source>Subtractive Ellipsoid</source>
      <translation>減算楕円体</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="437"/>
      <source>Subtractive Torus</source>
      <translation>減算トーラス</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="443"/>
      <source>Subtractive Prism</source>
      <translation>減算角柱</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="449"/>
      <source>Subtractive Wedge</source>
      <translation>減算ウェッジ</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="335"/>
      <source>Edit Shape Binder</source>
      <translation>シェイプバインダーを編集</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="346"/>
      <source>Create Shape Binder</source>
      <translation>シェイプバインダーを作成</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="439"/>
      <source>Create Sub-Shape Binder</source>
      <translation>サブシェイプバインダーを作成</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="508"/>
      <source>Create Clone</source>
      <translation>クローンを作成</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1110"/>
      <source>Make Copy</source>
      <translation>コピーを作成</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2500"/>
      <source>Convert to Multi-Transform feature</source>
      <translation>マルチ変換フィーチャーに変換</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="253"/>
      <source>Sketch on Face</source>
      <translation>面上にスケッチ</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="314"/>
      <source>Make copy</source>
      <translation>コピーの作成</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="516"/>
      <location filename="../../SketchWorkflow.cpp" line="772"/>
      <source>New Sketch</source>
      <translation>新しいスケッチ</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2597"/>
      <source>Create Boolean</source>
      <translation>Bool変数の作成</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="222"/>
      <location filename="../../DlgActiveBody.cpp" line="101"/>
      <source>Add a Body</source>
      <translation>ボディーを追加</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="523"/>
      <source>Migrate legacy Part Design features to bodies</source>
      <translation>過去のパートデザインのフィーチャーをボディーに移行</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="769"/>
      <source>Duplicate a Part Design object</source>
      <translation>パートデザイン・オブジェクトを複製</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1110"/>
      <source>Move a feature inside body</source>
      <translation>ボディー内のフィーチャーを移動</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="723"/>
      <source>Move tip to selected feature</source>
      <translation>選択したフィーチャーにTIPを移動</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="926"/>
      <source>Move an object</source>
      <translation>オブジェクトを移動</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="258"/>
      <source>Mirror</source>
      <translation>鏡像</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="298"/>
      <source>Linear Pattern</source>
      <translation>直線状パターン</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="347"/>
      <source>Polar Pattern</source>
      <translation>軸周状パターン</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="386"/>
      <source>Scale</source>
      <translation>尺度</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="53"/>
      <source>Face Tools</source>
      <translation>表面ツール</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Edge Tools</source>
      <translation>エッジツール</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Boolean Tools</source>
      <translation>ブーリアン操作ツール</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Helper Tools</source>
      <translation>補助ツール</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Modeling Tools</source>
      <translation>モデリングツール</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Create Geometry</source>
      <translation>ジオメトリを作成</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute Parameter</source>
      <translation>インボリュートパラメーター</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth</source>
      <translation>歯数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module</source>
      <translation>モジュール</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle</source>
      <translation>圧力角</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision</source>
      <translation>高精度</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="139"/>
      <location filename="../../../InvoluteGearFeature.ui" line="166"/>
      <source>True</source>
      <translation>True</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="144"/>
      <location filename="../../../InvoluteGearFeature.ui" line="171"/>
      <source>False</source>
      <translation>False</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="152"/>
      <source>External gear</source>
      <translation>外歯車</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum coefficient</source>
      <translation>歯先のたけ係数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum coefficient</source>
      <translation>歯元のたけ係数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root fillet coefficient</source>
      <translation>ルートフィレット係数</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile shift coefficient</source>
      <translation>プロファイルシフト係数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgActiveBody</name>
    <message>
      <location filename="../../DlgActiveBody.ui" line="14"/>
      <source>Active Body Required</source>
      <translation>アクティブなボディーが必要です。</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="20"/>
      <source>To create a new Part Design object, there must be an active body in the document.
Select a body from below, or create a new body.</source>
      <translation>新しいパートデザイン・オブジェクトを作成するには、ドキュメント内にアクティブなボディーが存在する必要があります。

以下からボディーを選択するか、新しいボディーを作成してください。</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="35"/>
      <source>Create New Body</source>
      <translation>新しいボディーを作成</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="52"/>
      <source>Please select</source>
      <translation>選択してください</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="14"/>
      <source>Geometric Primitives</source>
      <translation>幾何プリミティブ</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>最初の方向の角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>2番目の方向の角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length</source>
      <translation>長さ</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="82"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="173"/>
      <source>Width</source>
      <translation>幅</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="193"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="287"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="505"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1254"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1580"/>
      <source>Height</source>
      <translation>高さ</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius</source>
      <translation>半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle</source>
      <translation>回転角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1</source>
      <translation>半径 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2</source>
      <translation>半径 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="551"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1620"/>
      <source>Angle</source>
      <translation>角度</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="674"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="896"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1091"/>
      <source>U parameter</source>
      <translation>Uパラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters</source>
      <translation>Vパラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>ローカルZ方向の半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local X-direction</source>
      <translation>ローカルX方向の半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3</source>
      <translation>半径 3</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local Y-direction
If zero, it is equal to Radius2</source>
      <translation>ローカルY方向の半径
ゼロの場合はRadius2と等しくなります</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter</source>
      <translation>Vパラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local XY-plane</source>
      <translation>ローカルXY平面における半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local XZ-plane</source>
      <translation>ローカルXZ平面における半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon</source>
      <translation>多角形</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius</source>
      <translation>外接円の半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max</source>
      <translation>X の最小値/最大値</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max</source>
      <translation>Y の最小値/最大値</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max</source>
      <translation>Z の最小値/最大値</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max</source>
      <translation>X2 の最小値/最大値</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max</source>
      <translation>Z2 の最小値/最大値</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch</source>
      <translation>ピッチ</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system</source>
      <translation>座標系</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth</source>
      <translation>増加率</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations</source>
      <translation>回転数</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1</source>
      <translation>角度 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2</source>
      <translation>角度 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From 3 Points</source>
      <translation>3点から</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius</source>
      <translation>長半径</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius</source>
      <translation>短半径</translation>
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
      <location filename="../../TaskPrimitiveParameters.ui" line="1645"/>
      <source>Right-handed</source>
      <translation>右手系</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>左手系</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2086"/>
      <source>Start point</source>
      <translation>始点</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2160"/>
      <source>End point</source>
      <translation>終点</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgReference</name>
    <message>
      <location filename="../../DlgReference.ui" line="14"/>
      <source>Reference</source>
      <translation>参照</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="20"/>
      <source>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</source>
      <translation>アクティブなボディの一部ではないジオメトリを選択しました。 これらの選択をどのように処理するかを定義してください。これらの参照を必要としない場合は、コマンドをキャンセルします。</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="42"/>
      <source>Make independent copy (recommended)</source>
      <translation>独立したコピーを作成（推奨）</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="52"/>
      <source>Make dependent copy</source>
      <translation>依存コピーを作成</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="59"/>
      <source>Create cross-reference</source>
      <translation>相互参照を作成</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::NoDependentsSelection</name>
    <message>
      <location filename="../../ReferenceSelection.cpp" line="285"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>これを選択すると依存関係の循環が発生します。</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add Body</source>
      <translation>ボディーを追加</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove Body</source>
      <translation>ボディーを削除</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="48"/>
      <source>Fuse</source>
      <translation>結合</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="53"/>
      <source>Cut</source>
      <translation>切り取り</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="58"/>
      <source>Common</source>
      <translation>共通部分</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="51"/>
      <source>Boolean Parameters</source>
      <translation>ブーリアンパラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="82"/>
      <source>Remove</source>
      <translation>削除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="47"/>
      <source>Primitive Parameters</source>
      <translation>プリミティブパラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="932"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="940"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="948"/>
      <source>Invalid wedge parameters</source>
      <translation>無効なウェッジパラメーターです。</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="933"/>
      <source>X min must not be equal to X max!</source>
      <translation>X minはX maxより小さい必要があります。</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="941"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>Y minはY maxより小さい必要があります。</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="949"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>Z minはZ maxは小さい必要があります。</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="991"/>
      <source>Create primitive</source>
      <translation>プリミティブを作成</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>選択モードとプレビューモードを切り替え</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="23"/>
      <source>Select</source>
      <translation>選択</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- 強調表示する項目を選択します
- 項目をダブルクリックして面取りを表示します</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="48"/>
      <source>Type</source>
      <translation>タイプ</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="56"/>
      <source>Equal distance</source>
      <translation>等距離</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="61"/>
      <source>Two distances</source>
      <translation>2つの距離</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="66"/>
      <source>Distance and angle</source>
      <translation>距離と角度</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="79"/>
      <source>Flips the direction</source>
      <translation>方向を反転</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="119"/>
      <source>Use all edges</source>
      <translation>すべてのエッジを使用</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="100"/>
      <source>Size</source>
      <translation>サイズ</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="146"/>
      <source>Size 2</source>
      <translation>サイズ2</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="179"/>
      <source>Angle</source>
      <translation>角度</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="344"/>
      <source>Empty chamfer created!
</source>
      <translation>空の面取りが作成されました！
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="384"/>
      <source>Empty body list</source>
      <translation>ボディーのリストを空にする</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="384"/>
      <source>The body list cannot be empty</source>
      <translation>このボディーのリストは空にできません</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="399"/>
      <source>Boolean: Accept: Input error</source>
      <translation>ブーリアン: 許可: 入力エラー</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="105"/>
      <source>Incompatible Reference Set</source>
      <translation>互換性のない参照セットです。</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="107"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>現在の参照セットに合うアタッチメントモードがありません。続行を選択した場合、フィーチャーは今のまま残り、参照変更による移動は行われません。続行しますか？</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="228"/>
      <source>The feature could not be created with the given parameters.
The geometry may be invalid or the parameters may be incompatible.
Please adjust the parameters and try again.</source>
      <translation>指定されたパラメーターでフィーチャーを作成できませんでした。
ジオメトリーが無効であるか、パラメーターがおかしい可能性があります。
パラメーターを調整し再度実行してください。</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="235"/>
      <source>Input error</source>
      <translation>入力エラー</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="440"/>
      <source>Input error</source>
      <translation>入力エラー</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>選択モードとプレビューモードを切り替え</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="23"/>
      <source>Select</source>
      <translation>選択</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- 強調表示する項目を選択します
- 項目をダブルクリックして下書きを表示します</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="46"/>
      <source>Draft angle</source>
      <translation>抜き勾配の角度</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="79"/>
      <source>Neutral Plane</source>
      <translation>中立面</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="96"/>
      <source>Pull Direction</source>
      <translation>引き抜き方向</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="111"/>
      <source>Reverse pull direction</source>
      <translation>引き抜き方向を反転</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="288"/>
      <source>Empty draft created!
</source>
      <translation>空の抜き勾配が作成されました！
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="298"/>
      <source>Select</source>
      <translation>選択</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="303"/>
      <source>Confirm Selection</source>
      <translation>選択を確認</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="316"/>
      <source>Add All Edges</source>
      <translation>すべてのエッジを追加</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="322"/>
      <source>Adds all edges to the list box (only when in add selection mode)</source>
      <translation>リストボックスにすべてのエッジを追加 (追加選択モード時のみ)</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="331"/>
      <source>Remove</source>
      <translation>削除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1372"/>
      <source>No face selected</source>
      <translation>面が選択されていません</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="171"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1141"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="75"/>
      <source>Remove</source>
      <translation>削除</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="352"/>
      <source>Preview</source>
      <translation>プレビュー</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="356"/>
      <source>Select Faces</source>
      <translation>面を選択</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="692"/>
      <source>Select reference…</source>
      <translation>参照を選択...</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="602"/>
      <source>No shape selected</source>
      <translation>シェイプが選択されていません</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="685"/>
      <source>Sketch normal</source>
      <translation>スケッチ法線</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="688"/>
      <source>Face normal</source>
      <translation>面の法線</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="696"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="699"/>
      <source>Custom direction</source>
      <translation>カスタム方向</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1088"/>
      <source>Click on a shape in the model</source>
      <translation>モデルのシェイプをクリック</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1359"/>
      <source>One sided</source>
      <translation>片側</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1360"/>
      <source>Two sided</source>
      <translation>両側</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1361"/>
      <source>Symmetric</source>
      <translation>対称</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1367"/>
      <source>Click on a face in the model</source>
      <translation>モデルの面をクリック</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeaturePick</name>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="23"/>
      <source>Allow used features</source>
      <translation>使用されているフィーチャーを許可</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="30"/>
      <source>Allow External Features</source>
      <translation>外部フィーチャーを許可</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="42"/>
      <source>From other bodies of the same part</source>
      <translation>同じパーツの他のボディーから</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="49"/>
      <source>From different parts or free features</source>
      <translation>異なるパーツまたは自由なフィーチャーから</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="66"/>
      <source>Make independent copy (recommended)</source>
      <translation>独立したコピーを作成（推奨）</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="79"/>
      <source>Make dependent copy</source>
      <translation>依存コピーを作成</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>相互参照を作成</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="67"/>
      <source>Valid</source>
      <translation>有効</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="69"/>
      <source>Invalid shape</source>
      <translation>無効な形状です。</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="71"/>
      <source>No wire in sketch</source>
      <translation>スケッチ上にワイヤーがありません。</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="73"/>
      <source>Sketch already used by other feature</source>
      <translation>スケッチはすでに他の形状に使用されています。</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="75"/>
      <source>Belongs to another body</source>
      <translation>他のボディーに属しています。</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="77"/>
      <source>Belongs to another part</source>
      <translation>他のパーツに属しています。</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="79"/>
      <source>Doesn't belong to any body</source>
      <translation>どのボディーにも属していません。</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="81"/>
      <source>Base plane</source>
      <translation>ベース平面</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="83"/>
      <source>Feature is located after the tip of the body</source>
      <translation>フィーチャーはボディーのTIPの後に位置しています。</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="95"/>
      <source>Select attachment</source>
      <translation>アタッチメントを選択</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>選択モードとプレビューモードを切り替え</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="23"/>
      <source>Select</source>
      <translation>選択</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>- 強調表示する項目を選択します
- 項目をダブルクリックしてフィレットを表示します</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="46"/>
      <source>Radius</source>
      <translation>半径</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="62"/>
      <source>Use all edges</source>
      <translation>すべてのエッジを使用</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="203"/>
      <source>Empty fillet created!</source>
      <translation>空のフィレットが作成されました!</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>有効</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="239"/>
      <source>Base X-axis</source>
      <translation>ベースX軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="240"/>
      <source>Base Y-axis</source>
      <translation>ベースY軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="241"/>
      <source>Base Z-axis</source>
      <translation>ベースZ軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="223"/>
      <source>Horizontal sketch axis</source>
      <translation>水平スケッチ軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="222"/>
      <source>Vertical sketch axis</source>
      <translation>垂直スケッチ軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="221"/>
      <source>Normal sketch axis</source>
      <translation>通常のスケッチ軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status</source>
      <translation>ステータス</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis</source>
      <translation>軸</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="206"/>
      <source>Select reference…</source>
      <translation>参照を選択...</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode</source>
      <translation>モード</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="98"/>
      <source>Pitch-Height-Angle</source>
      <translation>ピッチ-高さ-角度</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="103"/>
      <source>Pitch-Turns-Angle</source>
      <translation>ピッチ-ターン-角度</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="108"/>
      <source>Height-Turns-Angle</source>
      <translation>高さ-ターン-角度</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>高さ-ターン-伸び</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="125"/>
      <source>Pitch</source>
      <translation>ピッチ</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height</source>
      <translation>高さ</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns</source>
      <translation>巻き数</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle</source>
      <translation>円錐の角度</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth</source>
      <translation>半径方向の増加率</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="289"/>
      <source>Recompute on change</source>
      <translation>変更時に再計算</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="255"/>
      <source>Left handed</source>
      <translation>左手系</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="265"/>
      <source>Reversed</source>
      <translation>逆方向</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="272"/>
      <source>Remove outside of profile</source>
      <translation>プロファイルの外側を削除</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="55"/>
      <source>Helix Parameters</source>
      <translation>らせんパラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="225"/>
      <source>Construction line %1</source>
      <translation>補助線 %1</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="293"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>警告: らせんが自己交差している可能性があります。</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="298"/>
      <source>Error: helix touches itself</source>
      <translation>エラー: らせんが自身に接触しています。</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="347"/>
      <source>Error: unsupported mode</source>
      <translation>エラー: サポートされていないモードです。</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="55"/>
      <source>Counterbore</source>
      <translation>カウンターボア</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="56"/>
      <source>Countersink</source>
      <translation>皿穴</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="57"/>
      <source>Counterdrill</source>
      <translation>座ぐり</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="61"/>
      <source>Hole Parameters</source>
      <translation>穴パラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="70"/>
      <source>None</source>
      <translation>なし</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="71"/>
      <source>ISO metric regular</source>
      <translation>ISOメートル標準</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="72"/>
      <source>ISO metric fine</source>
      <translation>ISOメートル高精度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="73"/>
      <source>UTS coarse</source>
      <translation>UTS低精度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="74"/>
      <source>UTS fine</source>
      <translation>UTS高精度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="75"/>
      <source>UTS extra fine</source>
      <translation>UTS超高精度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="76"/>
      <source>ANSI pipes</source>
      <translation>ANSIパイプ</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="77"/>
      <source>ISO/BSP pipes</source>
      <translation>ISO/BSPパイプ</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="78"/>
      <source>BSW whitworth</source>
      <translation>BSWウィットウォース</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="79"/>
      <source>BSF whitworth fine</source>
      <translation>BSWウィットウォース高精度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="80"/>
      <source>ISO tyre valves</source>
      <translation>ISOタイヤバルブ</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="678"/>
      <source>Medium</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>中</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="682"/>
      <source>Fine</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>細かい</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="686"/>
      <source>Coarse</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>粗い</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="692"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>標準</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="696"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>閉じる</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="700"/>
      <source>Loose</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>あそび</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="704"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>標準</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="705"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>閉じる</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="706"/>
      <source>Wide</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>幅</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>線織面</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="27"/>
      <source>Closed</source>
      <translation>閉じる</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>プロファイル</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="42"/>
      <source>Object</source>
      <translation>オブジェクト</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="65"/>
      <source>Add Section</source>
      <translation>セクションを追加</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="78"/>
      <source>Remove Section</source>
      <translation>セクションを削除</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="103"/>
      <source>List can be reordered by dragging</source>
      <translation>リストをドラッグして並べ替えることができます</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="120"/>
      <source>Recompute on change</source>
      <translation>変更時に再計算</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="48"/>
      <source>Loft Parameters</source>
      <translation>ロフトパラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="72"/>
      <source>Remove</source>
      <translation>削除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMirroredParameters</name>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="34"/>
      <source>Plane</source>
      <translation>平面</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="184"/>
      <source>Error</source>
      <translation>エラー</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMultiTransformParameters</name>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="32"/>
      <source>Transformations</source>
      <translation>配置変換</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="52"/>
      <source>OK</source>
      <translation>OK</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="69"/>
      <source>Edit</source>
      <translation>編集</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="72"/>
      <source>Delete</source>
      <translation>削除</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="75"/>
      <source>Add Mirror Transformation</source>
      <translation>鏡像変換を追加</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="83"/>
      <source>Add Linear Pattern</source>
      <translation>直線状パターンを追加</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="91"/>
      <source>Add Polar Pattern</source>
      <translation>軸周状パターンを追加</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="99"/>
      <source>Add Scale Transformation</source>
      <translation>拡大縮小変換を追加</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="102"/>
      <source>Move Up</source>
      <translation>上へ移動</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="105"/>
      <source>Move Down</source>
      <translation>下へ移動</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="137"/>
      <source>Right-click to add a transformation</source>
      <translation>右クリックで変換を追加</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="38"/>
      <source>Pad Parameters</source>
      <translation>パッドパラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Offset the pad from the face at which the pad will end on side 1</source>
      <translation>パッドの第1方向の端面からパッドをオフセット</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="41"/>
      <source>Offset the pad from the face at which the pad will end on side 2</source>
      <translation>パッドの第2方向の端面からパッドをオフセット</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Reverses pad direction</source>
      <translation>パッドの方向を反転</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="71"/>
      <source>Dimension</source>
      <translation>寸法</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="72"/>
      <source>To last</source>
      <translation>最後まで</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>To first</source>
      <translation>最初まで</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>Up to face</source>
      <translation>面まで</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>Up to shape</source>
      <translation>シェイプまで</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadPocketParameters</name>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="68"/>
      <location filename="../../TaskPadPocketParameters.ui" line="303"/>
      <source>Type</source>
      <translation>タイプ</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="76"/>
      <source>Dimension</source>
      <translation>寸法</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="84"/>
      <location filename="../../TaskPadPocketParameters.ui" line="313"/>
      <source>Length</source>
      <translation>長さ</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="101"/>
      <location filename="../../TaskPadPocketParameters.ui" line="330"/>
      <source>Offset to face</source>
      <translation>面に対するオフセット</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="192"/>
      <location filename="../../TaskPadPocketParameters.ui" line="421"/>
      <source>Select all faces</source>
      <translation>すべての面を選択</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="217"/>
      <location filename="../../TaskPadPocketParameters.ui" line="446"/>
      <source>Select</source>
      <translation>選択</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="255"/>
      <location filename="../../TaskPadPocketParameters.ui" line="484"/>
      <source>Select Face</source>
      <translation>面を選択</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="281"/>
      <source>Side 2</source>
      <translation>第2方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="512"/>
      <source>Direction</source>
      <translation>方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="541"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>方向を設定するか、参照としてモデルからエッジを選択</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="546"/>
      <source>Sketch normal</source>
      <translation>スケッチ法線</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="556"/>
      <source>Custom direction</source>
      <translation>カスタム方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="569"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>パッド方向にカスタムベクトルを使用します。そうでなければ
スケッチ平面の法線ベクトルが使用されます。</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="521"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>チェックされていない場合、長さは指定された方向に沿って
測定されます</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="525"/>
      <source>Length along sketch normal</source>
      <translation>スケッチ法線に沿った長さ</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="214"/>
      <location filename="../../TaskPadPocketParameters.ui" line="443"/>
      <source>Toggles between selection and preview mode</source>
      <translation>選択モードとプレビューモードを切り替え</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="505"/>
      <source>Reversed</source>
      <translation>逆方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="534"/>
      <source>Direction/edge</source>
      <translation>方向/エッジ</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="551"/>
      <source>Select reference…</source>
      <translation>参照を選択...</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="582"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="589"/>
      <source>X-component of direction vector</source>
      <translation>方向ベクトルのX成分</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="611"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="618"/>
      <source>Y-component of direction vector</source>
      <translation>方向ベクトルのY成分</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="640"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="647"/>
      <source>Z-component of direction vector</source>
      <translation>方向ベクトルのZ成分</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="118"/>
      <location filename="../../TaskPadPocketParameters.ui" line="347"/>
      <source>Angle to taper the extrusion</source>
      <translation>押し出しテーパー角度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Mode</source>
      <translation>モード</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="46"/>
      <source>Side 1</source>
      <translation>第1方向</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="121"/>
      <location filename="../../TaskPadPocketParameters.ui" line="350"/>
      <source>Taper angle</source>
      <translation>テーパー角度</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="174"/>
      <location filename="../../TaskPadPocketParameters.ui" line="403"/>
      <source>Select Shape</source>
      <translation>シェイプを選択</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="189"/>
      <location filename="../../TaskPadPocketParameters.ui" line="418"/>
      <source>Selects all faces of the shape</source>
      <translation>シェイプのすべての面を選択</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="678"/>
      <source>Recompute on change</source>
      <translation>変更時に再計算</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeOrientation</name>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="22"/>
      <source>Orientation mode</source>
      <translation>方向モード</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="36"/>
      <source>Standard</source>
      <translation>標準</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="41"/>
      <source>Fixed</source>
      <translation>固定</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="46"/>
      <source>Frenet</source>
      <translation>フレネ</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="51"/>
      <source>Auxiliary</source>
      <translation>補助</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="56"/>
      <source>Binormal</source>
      <translation>従法線</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="76"/>
      <source>Curvilinear equivalence</source>
      <translation>曲線等量</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="83"/>
      <source>Profile</source>
      <translation>プロファイル</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="91"/>
      <source>Object</source>
      <translation>オブジェクト</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="125"/>
      <source>Add Edge</source>
      <translation>エッジを追加</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="144"/>
      <source>Remove Edge</source>
      <translation>エッジを削除</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="166"/>
      <source>Set the constant binormal vector used to calculate the profiles orientation</source>
      <translation>プロファイルの方向を計算するのに使用される定従法線ベクトルを設定</translation>
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
      <location filename="../../TaskPipeParameters.cpp" line="575"/>
      <source>Section Orientation</source>
      <translation>断面方向</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="603"/>
      <source>Remove</source>
      <translation>削除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeParameters</name>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="20"/>
      <source>Profile</source>
      <translation>プロファイル</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="28"/>
      <location filename="../../TaskPipeParameters.ui" line="93"/>
      <source>Object</source>
      <translation>オブジェクト</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="51"/>
      <source>Corner transition</source>
      <translation>角の遷移</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right corner</source>
      <translation>直角の角</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round corner</source>
      <translation>丸い角</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to Sweep Along</source>
      <translation>スイープ経路</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add edge</source>
      <translation>エッジを追加</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove edge</source>
      <translation>エッジを削除</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>変換</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="67"/>
      <source>Pipe Parameters</source>
      <translation>パイプパラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="86"/>
      <source>Remove</source>
      <translation>削除</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="444"/>
      <location filename="../../TaskPipeParameters.cpp" line="561"/>
      <source>Input error</source>
      <translation>入力エラー</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="444"/>
      <source>No active body</source>
      <translation>アクティブなボディーがありません。</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeScaling</name>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="22"/>
      <source>Transform mode</source>
      <translation>変換モード</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="36"/>
      <source>Constant</source>
      <translation>定数</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="41"/>
      <source>Multisection</source>
      <translation>マルチ断面</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="64"/>
      <source>Add Section</source>
      <translation>セクションを追加</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="77"/>
      <source>Remove Section</source>
      <translation>セクションを削除</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="102"/>
      <source>List can be reordered by dragging</source>
      <translation>リストをドラッグして並べ替えることができます</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="870"/>
      <source>Section Transformation</source>
      <translation>断面変換</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="889"/>
      <source>Remove</source>
      <translation>削除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="38"/>
      <source>Pocket Parameters</source>
      <translation>ポケットパラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="41"/>
      <source>Offset from the selected face at which the pocket will end on side 1</source>
      <translation>ポケットの第1方向の選択端面からオフセット</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="44"/>
      <source>Offset from the selected face at which the pocket will end on side 2</source>
      <translation>ポケットの第2方向の選択端面からオフセット</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="46"/>
      <source>Reverses pocket direction</source>
      <translation>ポケットの方向を反転</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="75"/>
      <source>Dimension</source>
      <translation>寸法</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="76"/>
      <source>Through all</source>
      <translation>貫通</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="77"/>
      <source>To first</source>
      <translation>最初まで</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="78"/>
      <source>Up to face</source>
      <translation>面まで</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="79"/>
      <source>Up to shape</source>
      <translation>シェイプまで</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="22"/>
      <source>Type</source>
      <translation>タイプ</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="50"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="254"/>
      <source>Base X-axis</source>
      <translation>ベースX軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="255"/>
      <source>Base Y-axis</source>
      <translation>ベースY軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="60"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="256"/>
      <source>Base Z-axis</source>
      <translation>ベースZ軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="65"/>
      <source>Horizontal sketch axis</source>
      <translation>水平スケッチ軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="70"/>
      <source>Vertical sketch axis</source>
      <translation>垂直スケッチ軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="154"/>
      <source>Symmetric to plane</source>
      <translation>面に対して対称</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="161"/>
      <source>Reversed</source>
      <translation>逆方向</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="120"/>
      <source>2nd angle</source>
      <translation>2番目の角度</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="42"/>
      <source>Axis</source>
      <translation>軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="75"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="264"/>
      <source>Select reference…</source>
      <translation>参照を選択...</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="87"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="197"/>
      <source>Angle</source>
      <translation>角度</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="170"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="160"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="491"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="192"/>
      <source>Recompute on change</source>
      <translation>変更時に再計算</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="199"/>
      <source>To last</source>
      <translation>最後まで</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="202"/>
      <source>Through all</source>
      <translation>貫通</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="204"/>
      <source>To first</source>
      <translation>最初まで</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="215"/>
      <source>Up to face</source>
      <translation>面まで</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="216"/>
      <source>Two angles</source>
      <translation>2つの角度</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="479"/>
      <source>No face selected</source>
      <translation>面が選択されていません</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskScaledParameters</name>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="34"/>
      <source>Factor</source>
      <translation>係数</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="48"/>
      <source>Occurrences</source>
      <translation>回数</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="22"/>
      <source>Object</source>
      <translation>オブジェクト</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="48"/>
      <source>Add Geometry</source>
      <translation>ジオメトリを追加</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="67"/>
      <source>Remove Geometry</source>
      <translation>ジオメトリを削除</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="59"/>
      <source>Shape Binder Parameters</source>
      <translation>シェイプバインダーパラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="137"/>
      <source>Remove</source>
      <translation>削除</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="193"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>選択モードとプレビューモードを切り替え</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="23"/>
      <source>Select</source>
      <translation>選択</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>- 強調表示する項目を選択します
- 項目をダブルクリックしてフィーチャを表示します</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="46"/>
      <source>Thickness</source>
      <translation>厚み</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="75"/>
      <source>Mode</source>
      <translation>モード</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="83"/>
      <source>Skin</source>
      <translation>スキン</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="88"/>
      <source>Pipe</source>
      <translation>パイプ</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="93"/>
      <source>Recto verso</source>
      <translation>レクト・ベルソ</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="101"/>
      <source>Join type</source>
      <translation>接合の種類</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="109"/>
      <source>Arc</source>
      <translation>円弧</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="114"/>
      <location filename="../../TaskThicknessParameters.ui" line="124"/>
      <source>Intersection</source>
      <translation>共通集合</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="131"/>
      <source>Make thickness inwards</source>
      <translation>内側に向かって厚みを作成</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="267"/>
      <source>Empty thickness created!
</source>
      <translation>空の厚みが作成されました！
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="111"/>
      <source>Remove</source>
      <translation>削除</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="404"/>
      <source>Normal sketch axis</source>
      <translation>通常のスケッチ軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="403"/>
      <source>Vertical sketch axis</source>
      <translation>垂直スケッチ軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="402"/>
      <source>Horizontal sketch axis</source>
      <translation>水平スケッチ軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="406"/>
      <location filename="../../TaskTransformedParameters.cpp" line="442"/>
      <source>Construction line %1</source>
      <translation>補助線 %1</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="420"/>
      <source>Base X-axis</source>
      <translation>ベースX軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="421"/>
      <source>Base Y-axis</source>
      <translation>ベースY軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="422"/>
      <source>Base Z-axis</source>
      <translation>ベースZ軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="456"/>
      <source>Base XY-plane</source>
      <translation>ベースXY平面</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="457"/>
      <source>Base YZ-plane</source>
      <translation>ベースYZ平面</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="458"/>
      <source>Base XZ-plane</source>
      <translation>ベースXZ平面</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="430"/>
      <location filename="../../TaskTransformedParameters.cpp" line="466"/>
      <source>Select reference…</source>
      <translation>参照を選択...</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="35"/>
      <source>Transform body</source>
      <translation>ボディーを変換</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="48"/>
      <source>Transform tool shapes</source>
      <translation>ツールシェイプを変換</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="78"/>
      <source>Add Feature</source>
      <translation>フィーチャーを追加</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="88"/>
      <source>Remove Feature</source>
      <translation>フィーチャーを削除</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="122"/>
      <source>Recompute on change</source>
      <translation>変更時に再計算</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="106"/>
      <source>List can be reordered by dragging</source>
      <translation>リストをドラッグして並べ替えることができます</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="908"/>
      <source>Select Body</source>
      <translation>ボディーを選択</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="909"/>
      <source>Select a body from the list</source>
      <translation>リストからボディーを選択</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1095"/>
      <source>Move Feature After…</source>
      <translation>フィーチャーを次の後ろへ移動…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1096"/>
      <source>Select a feature from the list</source>
      <translation>リストからフィーチャーを選択</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1183"/>
      <source>Move Tip</source>
      <translation>TIPを移動</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1189"/>
      <source>Set tip to last feature?</source>
      <translation>最後のフィーチャーにTIPを設定しますか？</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1184"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>移動したフィーチャは、現在設定されているTIPの後に表示されます。</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="146"/>
      <source>Invalid selection</source>
      <translation>無効な選択です。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="147"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>選択したオブジェクトに合うアタッチメントモードがありません。他のものを選択してください。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="160"/>
      <location filename="../../Command.cpp" line="168"/>
      <location filename="../../Command.cpp" line="175"/>
      <source>Error</source>
      <translation>エラー</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="809"/>
      <source>Several sub-elements selected</source>
      <translation>いくつかのサブ要素が選択されています</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="810"/>
      <source>Select a single face as support for a sketch!</source>
      <translation>スケッチのサポートとして1つの面を選択してください！</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="817"/>
      <source>Select a face as support for a sketch!</source>
      <translation>スケッチのサポートとして面を選択してください！</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="824"/>
      <source>Need a planar face as support for a sketch!</source>
      <translation>スケッチのサポートとして平らな面が必要です！</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="831"/>
      <source>Create a plane first or select a face to sketch on</source>
      <translation>まず平面を作成するか、またはスケッチを描く面を選択してください。</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="816"/>
      <source>No support face selected</source>
      <translation>サポート面が選択されていません</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="823"/>
      <source>No planar support</source>
      <translation>平面のサポートがありません</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="830"/>
      <source>No valid planes in this document</source>
      <translation>このドキュメントには有効な平面がありません。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="257"/>
      <location filename="../../ViewProvider.cpp" line="135"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="95"/>
      <location filename="../../Command.cpp" line="1138"/>
      <location filename="../../SketchWorkflow.cpp" line="728"/>
      <source>A dialog is already open in the task panel</source>
      <translation>タスクパネルで既にダイアログが開かれています</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="992"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>このコマンドを使うことはできません。このコマンドから引くソリッドがないためです。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="995"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>減算コマンドを実行する前に、ボディにフィーチャーが含まれていることを確認します。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1019"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>選択したオブジェクトを使用できません。選択したオブジェクトはアクティブなボディーに属している必要があります。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="161"/>
      <source>There is no active body. Please activate a body before inserting a datum entity.</source>
      <translation>アクティブなボディーがありません。データムエンティティを挿入する前にボディーをアクティブにしてください。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="467"/>
      <source>Sub-shape binder</source>
      <translation>サブシェイプバインダー</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1051"/>
      <source>No sketch to work on</source>
      <translation>作業のためのスケッチがありません。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1052"/>
      <source>No sketch is available in the document</source>
      <translation>ドキュメント内に使用可能なスケッチがありません。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="258"/>
      <location filename="../../ViewProvider.cpp" line="136"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="96"/>
      <location filename="../../Command.cpp" line="1139"/>
      <location filename="../../SketchWorkflow.cpp" line="729"/>
      <source>Close this dialog?</source>
      <translation>このダイアログを閉じますか？</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1821"/>
      <location filename="../../Command.cpp" line="1856"/>
      <source>Wrong selection</source>
      <translation>誤った選択</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1822"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>単一のボディからエッジ、面、またはボディを選択してください。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1829"/>
      <location filename="../../Command.cpp" line="2191"/>
      <source>Selection is not in the active body</source>
      <translation>アクティブなボディーが選択されていません。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1857"/>
      <source>Shape of the selected part is empty</source>
      <translation>選択したパートのシェイプが空です。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1830"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>アクティブなボディーからエッジ、面、またはボディを選択してください。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1022"/>
      <source>Consider using a shape binder or a base feature to reference external geometry in a body</source>
      <translation>ボディー内の外部ジオメトリーを参照するためにシェイプバインダーまたはベースフィーチャーを使用することを検討してください。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1843"/>
      <source>Wrong object type</source>
      <translation>間違ったオブジェクトの種類</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1844"/>
      <source>%1 works only on parts.</source>
      <translation>%1は部品にのみ適用できます。</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2192"/>
      <source>Please select only one feature in an active body.</source>
      <translation>アクティブなボディー内にあるフィーチャーを1つだけ選択してください。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="71"/>
      <source>Part creation failed</source>
      <translation>パーツ作成失敗</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="72"/>
      <source>Failed to create a part object.</source>
      <translation>パーツオブジェクトの作成に失敗しました。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="125"/>
      <location filename="../../CommandBody.cpp" line="133"/>
      <location filename="../../CommandBody.cpp" line="149"/>
      <location filename="../../CommandBody.cpp" line="215"/>
      <source>Bad base feature</source>
      <translation>不正なベースフィーチャー</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="126"/>
      <source>A body cannot be based on a Part Design feature.</source>
      <translation>パートデザイン・フィーチャーはボディーのベースにできません。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="134"/>
      <source>%1 already belongs to a body and cannot be used as a base feature for another body.</source>
      <translation>%1は既にボディーに属していて、別のボディーのベースフィーチャーとして使用できません。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="150"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>ベースフィーチャー (%1) は他のパーツに属しています。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="177"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>選択されているシェイプは複数のソリッドで構成されています。これは予期しない結果につながる可能性があります。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="183"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>選択されているシェイプは複数のシェルで構成されています。これは予期しない結果につながる可能性があります。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="189"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>選択されているシェイプはシェルひとつだけで構成されています。これは予期しない結果につながる可能性があります。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="195"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>選択されているシェイプは複数のソリッドまたはシェルで構成されています。これは予期しない結果につながる可能性があります。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="204"/>
      <source>Base feature</source>
      <translation>ベースフィーチャー</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="216"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>ボディーはベースとなるフィーチャーを1つだけ持ちます。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="230"/>
      <source>Body</source>
      <translation>Body</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="421"/>
      <source>Nothing to migrate</source>
      <translation>移行対象がありません。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="692"/>
      <source>Select exactly one Part Design feature or a body.</source>
      <translation>パートデザイン・フィーチャーまたはボディーを1つだけ選択してください。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="700"/>
      <source>Could not determine a body for the selected feature '%s'.</source>
      <translation>選択したフィーチャー「%s」のボディーを特定できませんでした。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="877"/>
      <source>Only features of a single source body can be moved</source>
      <translation>動かせるのは単一ボディーに含まれるフィーチャーだけです。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="616"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>スケッチ平面を移行できません。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="422"/>
      <source>No Part Design features without body found Nothing to migrate.</source>
      <translation>ボディーのないパートデザインフィーチャーが見つかりません。移行は行われません。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="617"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>'%1'を編集してベースまたはデータム平面をスケッチ平面として使用するよう再定義してください。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="691"/>
      <location filename="../../CommandBody.cpp" line="699"/>
      <location filename="../../CommandBody.cpp" line="711"/>
      <location filename="../../CommandBody.cpp" line="1061"/>
      <location filename="../../CommandBody.cpp" line="1071"/>
      <source>Selection error</source>
      <translation>選択エラー</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="712"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>ボディーのTIPにできるのはソリッドフィーチャーだけです。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="846"/>
      <location filename="../../CommandBody.cpp" line="876"/>
      <location filename="../../CommandBody.cpp" line="894"/>
      <source>Features cannot be moved</source>
      <translation>フィーチャーを移動できません。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="847"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>選択したフィーチャーの一部がソースボディーに依存しています。　</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="895"/>
      <source>There are no other bodies to move to</source>
      <translation>移動先となる他のボディーがありません。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1062"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>ボディーのベースフィーチャーを動かすことはできません。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1072"/>
      <source>Select one or more features from the same body.</source>
      <translation>同一のボディーから1つ以上のフィーチャーを選択してください。</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1087"/>
      <source>Beginning of the body</source>
      <translation>ボディーの先頭</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1168"/>
      <source>Dependency violation</source>
      <translation>依存関係の違反</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1169"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>初期機能は後の機能に依存してはいけません。

</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="307"/>
      <source>No previous feature found</source>
      <translation>前のフィーチャーが見つかりませんでした。</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="308"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>利用可能なベースフィーチャーがない場合、減算フィーチャーは作成できません。</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="439"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="240"/>
      <source>Vertical sketch axis</source>
      <translation>垂直スケッチ軸</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="440"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="241"/>
      <source>Horizontal sketch axis</source>
      <translation>水平スケッチ軸</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="243"/>
      <source>Construction line %1</source>
      <translation>補助線 %1</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="85"/>
      <source>Face</source>
      <translation>面</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="206"/>
      <source>Active Body Required</source>
      <translation>アクティブなボディーが必要です。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="148"/>
      <source>To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</source>
      <translation>パートデザインを使用するには、ドキュメント内にアクティブなボディーが必要です。ボディーを有効にする（ダブルクリック）か、新しいボディーを作成してください。

ボディーの無いパートデザイン・オブジェクトを持つ古いドキュメントの場合は、パートデザインの移行機能を使用してそれらをボディー内に配置してください。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="207"/>
      <source>To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</source>
      <translation>新しいパートデザイン・オブジェクトを作成するには、ドキュメント内にアクティブなボディーが必要です。既存のボディーを有効にする（ダブルクリック）か、新しいボディーを作成してください。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="273"/>
      <source>Feature is not in a body</source>
      <translation>フィーチャーがボディー内にありません。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="274"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>このフィーチャーを使用するためにはそれがドキュメント内のボディーオブジェクトに属している必要があります。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="320"/>
      <source>Feature is not in a part</source>
      <translation>フィーチャーがパーツ内にありません。</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="321"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>このフィーチャーを使用するためにはそれがドキュメント内のパーツオブジェクトに属している必要があります。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="62"/>
      <location filename="../../ViewProviderTransformed.cpp" line="63"/>
      <location filename="../../ViewProvider.cpp" line="92"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="225"/>
      <source>Edit %1</source>
      <translation>%1を編集</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="105"/>
      <source>Set Face Colors</source>
      <translation>面の色を設定</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="112"/>
      <location filename="../../ViewProviderDatum.cpp" line="212"/>
      <source>Plane</source>
      <translation>平面</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="117"/>
      <location filename="../../ViewProviderDatum.cpp" line="207"/>
      <source>Line</source>
      <translation>線</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="122"/>
      <location filename="../../ViewProviderDatum.cpp" line="217"/>
      <source>Point</source>
      <translation>点</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="127"/>
      <source>Coordinate System</source>
      <translation>座標系</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="234"/>
      <source>Edit Datum</source>
      <translation>データムを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="91"/>
      <source>Feature error</source>
      <translation>フィーチャーのエラー</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="92"/>
      <source>%1 misses a base feature.
This feature is broken and cannot be edited.</source>
      <translation>%1のベースフィーチャーが見つかりません。
このフィーチャーは壊れていて編集できません。</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="220"/>
      <source>Edit Shape Binder</source>
      <translation>シェイプバインダーを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="350"/>
      <source>Synchronize</source>
      <translation>同期する</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="352"/>
      <source>Select Bound Object</source>
      <translation>バウンドオブジェクトを選択</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="154"/>
      <source>The document "%1" you are editing was designed with an old version of Part Design workbench.</source>
      <translation>編集中のドキュメント「%1」は過去のバージョンのパートデザインワークベンチで設計されています。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="161"/>
      <source>Migrate in order to use modern Part Design features?</source>
      <translation>新しい パートデザイン機能を使用するために移行しますか？</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="166"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</source>
      <translation>ドキュメント「%1」は過去のパートデザインからの移行処理途中か、またはわずかに壊れた構造を含んでいます。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="173"/>
      <source>Make the migration automatically?</source>
      <translation>自動的に移行しますか？</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="176"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>移行を選択した場合、このファイルは過去のバージョンのFreeCADで編集できなくなることに注意してください。移行しない場合、ボディーやパーツといった新しい PartDesign 機能は使用できず、作成した部品をアセンブリワークベンチで使用することもできません。「パートデザイン-移行...」を選択することで移行はいつでも行えます。</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="189"/>
      <source>Migrate Manually</source>
      <translation>手動で移行</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="67"/>
      <source>Edit Boolean</source>
      <translation>ブーリアンを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="40"/>
      <source>Edit Chamfer</source>
      <translation>面取りを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="41"/>
      <source>Edit Draft</source>
      <translation>抜き勾配を編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="40"/>
      <source>Edit Fillet</source>
      <translation>フィレットを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="43"/>
      <source>Edit Groove</source>
      <translation>グルーブを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="48"/>
      <source>Edit Helix</source>
      <translation>らせんを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="63"/>
      <source>Edit Hole</source>
      <translation>穴を編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="38"/>
      <source>Edit Linear Pattern</source>
      <translation>直線状パターンを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="65"/>
      <source>Edit Loft</source>
      <translation>ロフトを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="38"/>
      <source>Edit Mirror</source>
      <translation>ミラーを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="47"/>
      <source>Edit Multi-Transform</source>
      <translation>マルチ変換を編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="43"/>
      <source>Edit Pad</source>
      <translation>パッドを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="75"/>
      <source>Edit Pipe</source>
      <translation>パイプを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="45"/>
      <source>Edit Pocket</source>
      <translation>ポケットを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="38"/>
      <source>Edit Polar Pattern</source>
      <translation>軸周状パターンを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="49"/>
      <source>Edit Primitive</source>
      <translation>プリミティブを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="43"/>
      <source>Edit Revolution</source>
      <translation>回転押し出しを編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="38"/>
      <source>Edit Scale</source>
      <translation>拡大縮小を編集</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="40"/>
      <source>Edit Thickness</source>
      <translation>厚みを編集</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket Parameters</source>
      <translation>スプロケットパラメーター</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="26"/>
      <source>Number of teeth</source>
      <translation>歯数</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="52"/>
      <source>Sprocket reference</source>
      <translation>スプロケット参照</translation>
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
      <translation>変速機付き自転車</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="141"/>
      <source>Bicycle without derailleur</source>
      <translation>変速機なし自転車</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="219"/>
      <source>Chain pitch</source>
      <translation>チェーンのピッチ</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="266"/>
      <source>Chain roller diameter</source>
      <translation>チェーンローラーの直径</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="310"/>
      <source>Tooth width</source>
      <translation>歯の幅</translation>
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
      <translation>オートバイ 420</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="186"/>
      <source>Motorcycle 425</source>
      <translation>オートバイ 425</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="191"/>
      <source>Motorcycle 428</source>
      <translation>オートバイ 428</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="196"/>
      <source>Motorcycle 520</source>
      <translation>オートバイ 520</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="201"/>
      <source>Motorcycle 525</source>
      <translation>オートバイ 525</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="206"/>
      <source>Motorcycle 530</source>
      <translation>オートバイ 530</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="211"/>
      <source>Motorcycle 630</source>
      <translation>オートバイ 630</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="238"/>
      <source>0 in</source>
      <translation>0 in</translation>
    </message>
  </context>
  <context>
    <name>TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="813"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>ねじ山の変更を即時更新
計算に時間がかかる場合があるので注意</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1006"/>
      <source>Thread Depth</source>
      <translation>ねじ山の深さ</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1059"/>
      <source>Customize thread clearance</source>
      <translation>ねじ山の間隔をカスタマイズ</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="709"/>
      <source>Clearance</source>
      <translation>クリアランス</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="65"/>
      <source>Head type</source>
      <translation>ヘッドの種類</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="153"/>
      <source>Depth type</source>
      <translation>深さの種類</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="245"/>
      <source>Head diameter</source>
      <translation>ヘッドの直径</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="293"/>
      <source>Head depth</source>
      <translation>ヘッドの深さ</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="678"/>
      <source>Clearance / Passthrough</source>
      <translation>クリアランス / パススルー</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="683"/>
      <source>Tap drill (to be threaded)</source>
      <translation>タップドリル（ねじ切り用）</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="688"/>
      <source>Modeled thread</source>
      <translation>モデル ねじ山</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="696"/>
      <source>Hole type</source>
      <translation>穴の種類</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="817"/>
      <source>Update thread view</source>
      <translation>ネジ山の表示を更新</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1065"/>
      <source>Custom Clearance</source>
      <translation>カスタムクリアランス</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1078"/>
      <source>Custom Thread clearance value</source>
      <translation>カスタム ねじ山間隔値</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="868"/>
      <source>Direction</source>
      <translation>方向</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="49"/>
      <source>Size</source>
      <translation>サイズ</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="722"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation>ホールクリアランス
ねじ山のない穴のみ利用可能</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="88"/>
      <location filename="../../TaskHoleParameters.ui" line="727"/>
      <source>Standard</source>
      <translation>標準</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="732"/>
      <source>Close</source>
      <translation>閉じる</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="737"/>
      <source>Wide</source>
      <translation>幅</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="848"/>
      <source>Class</source>
      <translation>クラス</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="835"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation>穴用プロファイルに従ったねじ穴の公差等級</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="552"/>
      <source>Diameter</source>
      <translation>直径</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="574"/>
      <source>Hole diameter</source>
      <translation>穴の直径</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="507"/>
      <source>Depth</source>
      <translation>深さ</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Hole Parameters</source>
      <translation>穴パラメーター</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="95"/>
      <source>Base profile types</source>
      <translation>ベースプロファイルの種類</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="126"/>
      <source>Circles and arcs</source>
      <translation>円と円弧</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="131"/>
      <source>Points, circles and arcs</source>
      <translation>点、円、円弧</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="136"/>
      <source>Points</source>
      <translation>点群</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="170"/>
      <location filename="../../TaskHoleParameters.ui" line="976"/>
      <source>Dimension</source>
      <translation>寸法</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="175"/>
      <source>Through all</source>
      <translation>貫通</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="197"/>
      <source>Custom head values</source>
      <translation>カスタムヘッド値</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="367"/>
      <source>Drill angle</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>ドリル角度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="403"/>
      <source>Include in depth</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>深さを含める</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="610"/>
      <source>Switch direction</source>
      <translation>方向の切り替え</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="662"/>
      <source>&lt;b&gt;Threading&lt;/b&gt;</source>
      <translation>&lt;b&gt;ねじ切り&lt;/b&gt;</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="783"/>
      <source>Thread</source>
      <translation>ねじ山</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="892"/>
      <source>&amp;Right hand</source>
      <translation>右手(&amp;R)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="908"/>
      <source>&amp;Left hand</source>
      <translation>左手(&amp;L)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="957"/>
      <source>Thread Depth Type</source>
      <translation>ねじ山の深さの種類</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="971"/>
      <source>Hole depth</source>
      <translation>穴の深さ</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="981"/>
      <source>Tapped (DIN76)</source>
      <translation>タップ (DIN76)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="118"/>
      <source>Cut type for screw heads</source>
      <translation>ネジ頭のカットタイプ</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="191"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation>「タイプ」で定義された値を上書きする場合はチェック</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="306"/>
      <source>For countersinks this is the depth of
the screw's top below the surface</source>
      <translation>皿穴の場合、面の下のねじ上部の深さです。</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="462"/>
      <source>Countersink angle</source>
      <translation>皿穴の角度</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="399"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>止まり穴の深さでは錐先の大きさが考慮されます。</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="626"/>
      <source>Tapered</source>
      <translation>テーパー</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="639"/>
      <source>Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</source>
      <translation>穴のテーパー角度
90 度: 垂直穴
90度未満: 底部の穴半径が小
90度超: 底部の穴半径が大</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="607"/>
      <source>Reverses the hole direction</source>
      <translation>穴の方向を反転</translation>
    </message>
  </context>
  <context>
    <name>TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="25"/>
      <source>No message</source>
      <translation>メッセージなし</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="41"/>
      <source>&amp;Sketch</source>
      <translation>スケッチ(&amp;S)</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Part Design</source>
      <translation>パートデザイン(&amp;P)</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="44"/>
      <source>Datums</source>
      <translation>データム</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>Additive Features</source>
      <translation>加算フィーチャー</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Subtractive Features</source>
      <translation>減算フィーチャー</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Dress-Up Features</source>
      <translation>フォーチャーを装飾</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Transformation Features</source>
      <translation>変換フィーチャー</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Sprocket…</source>
      <translation>スプロケット...</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Involute Gear</source>
      <translation>インボリュート歯車</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Shaft Design Wizard</source>
      <translation>シャフト設計ウィザード</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Measure</source>
      <translation>計測</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="61"/>
      <source>Refresh</source>
      <translation>更新</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="62"/>
      <source>Toggle 3D</source>
      <translation>3D の切り替え</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="63"/>
      <source>Part Design Helper</source>
      <translation>Part Design ヘルパー</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="64"/>
      <source>Part Design Modeling</source>
      <translation>Part Design モデリング</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="46"/>
      <source>Length [mm]</source>
      <translation>長さ [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="47"/>
      <source>Diameter [mm]</source>
      <translation>直径 [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Inner diameter [mm]</source>
      <translation>内径 [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Constraint type</source>
      <translation>拘束タイプ</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Start edge type</source>
      <translation>エッジ始端タイプ</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>Start edge size</source>
      <translation>エッジ始端サイズ</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="52"/>
      <source>End edge type</source>
      <translation>エッジ終端タイプ</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="53"/>
      <source>End edge size</source>
      <translation>エッジ終端サイズ</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="67"/>
      <source>Shaft Wizard</source>
      <translation>シャフトウィザード</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="75"/>
      <source>Section 1</source>
      <translation>セクション 1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="76"/>
      <source>Section 2</source>
      <translation>セクション 2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="80"/>
      <source>Add column</source>
      <translation>列を追加</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="128"/>
      <source>Section %s</source>
      <translation>セクション %s</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="157"/>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="176"/>
      <source>None</source>
      <translation>なし</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="158"/>
      <source>Fixed</source>
      <translation>固定</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="159"/>
      <source>Force</source>
      <translation>加力</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="160"/>
      <source>Bearing</source>
      <translation>ベアリング</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="161"/>
      <source>Gear</source>
      <translation>ギア</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="162"/>
      <source>Pulley</source>
      <translation>プーリー</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="179"/>
      <source>Chamfer</source>
      <translation>面取り</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="180"/>
      <source>Fillet</source>
      <translation>フィレット</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="58"/>
      <source>All</source>
      <translation>すべて</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="118"/>
      <source>Missing Module</source>
      <translation>モジュールが見つかりません。</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="124"/>
      <source>The Plot add-on is not installed. Install it to enable this feature.</source>
      <translation>プロットアドオンがインストールされていません。この機能を有効にするにはインストールしてください。</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="251"/>
      <source>Shaft design wizard...</source>
      <translation>シャフト設計ウィザード...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="254"/>
      <source>Start the shaft design wizard</source>
      <translation>シャフト設計ウィザードを開始</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="403"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>リンクされたオブジェクトはパートデザインのフィーチャーではありません。</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="412"/>
      <source>Tip shape is empty</source>
      <translation>TIPのシェイプが空です。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="66"/>
      <source>BaseFeature link is not set</source>
      <translation>BaseFeatureリンクが設定されていません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="72"/>
      <source>BaseFeature must be a Part::Feature</source>
      <translation>BaseFeatureはPart::Featureでなければなりません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="82"/>
      <source>BaseFeature has an empty shape</source>
      <translation>BaseFeatureに空のシェイプがあります。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="75"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>BaseFeatureなしでブーリアン演算のカットを行うことはできません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="92"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>Part::Featureとその派生物以外ではブーリアン演算を行うことはできません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="104"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>無効なベースシェイプではブーリアン演算を行うことはできません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="192"/>
      <location filename="../../../App/FeatureDraft.cpp" line="332"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="159"/>
      <location filename="../../../App/FeatureFillet.cpp" line="140"/>
      <location filename="../../../App/FeatureHole.cpp" line="2067"/>
      <location filename="../../../App/FeatureLoft.cpp" line="331"/>
      <location filename="../../../App/FeatureLoft.cpp" line="375"/>
      <location filename="../../../App/FeaturePipe.cpp" line="480"/>
      <location filename="../../../App/FeaturePipe.cpp" line="529"/>
      <location filename="../../../App/FeatureRevolution.cpp" line="284"/>
      <location filename="../../../App/FeatureGroove.cpp" line="253"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="773"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="789"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="802"/>
      <source>Result has multiple solids: enable 'Allow Compound' in the active body.</source>
      <translation>結果に複数のソリッドがあります: アクティブなボディーで「コンパウンドを許可」を有効にしてください。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="114"/>
      <source>Tool shape is null</source>
      <translation>ツールシェイプが null です。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="141"/>
      <source>Unsupported boolean operation</source>
      <translation>サポートされていないブーリアン演算です。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="351"/>
      <source>Cannot create a pad with a total length of zero.</source>
      <translation>合計長さゼロのパッドは作成できません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="356"/>
      <source>Cannot create a pocket with a total length of zero.</source>
      <translation>合計長さゼロのポケットは作成できません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="704"/>
      <source>No extrusion geometry was generated.</source>
      <translation>押し出しジオメトリーは生成されませんでした。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="728"/>
      <source>Resulting fused extrusion is null.</source>
      <translation>結合された押し出し結果が無効です。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="368"/>
      <location filename="../../../App/FeaturePipe.cpp" line="521"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="139"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="764"/>
      <source>Resulting shape is not a solid</source>
      <translation>結果シェイプはソリッドではありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="172"/>
      <source>Failed to create chamfer</source>
      <translation>面取りを作成できませんでした。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="327"/>
      <location filename="../../../App/FeatureFillet.cpp" line="120"/>
      <source>Resulting shape is null</source>
      <translation>結果シェイプが null です。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="141"/>
      <source>No edges specified</source>
      <translation>エッジが指定されていません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="293"/>
      <source>Size must be greater than zero</source>
      <translation>サイズはゼロより大きくなければなりません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="304"/>
      <source>Size2 must be greater than zero</source>
      <translation>サイズ2はゼロより大きくなければなりません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="311"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>角度は0から180の間でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="95"/>
      <source>Fillet not possible on selected shapes</source>
      <translation>選択したシェイプではフィレットを作成できません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="103"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>フィレット半径は0より大きくなければなりません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="157"/>
      <source>Fillet operation failed. The selected edges may contain geometry that cannot be filleted together. Try filleting edges individually or with a smaller radius.</source>
      <translation>フィレット操作に失敗しました。選択したエッジに同時にはフィレットできないジオメトリーが含まれている可能性があります。個別にエッジをフィレットするか、半径を小さくしてみてください。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="101"/>
      <source>Angle of groove too large</source>
      <translation>グルーブの角度が大きすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="108"/>
      <source>Angle of groove too small</source>
      <translation>グルーブの角度が小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1719"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>要求されたフィーチャーを作成できません。原因は以下である可能性があります。
・ アクティブなボディーにベースシェイプが無く、削除されるマテリアルが存在しない。
・ 選択されたスケッチがアクティブなボディーに属していない。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="400"/>
      <source>Failed to obtain profile shape</source>
      <translation>プロファイルのシェイプを取得できませんでした。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="454"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>方向とスケッチの法線ベクトルが直交しているため作成に失敗しました。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="176"/>
      <location filename="../../../App/FeatureGroove.cpp" line="154"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="477"/>
      <source>Creating a face from sketch failed</source>
      <translation>スケッチから面を作成できませんでした。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureGroove.cpp" line="115"/>
      <source>Angles of groove nullify each other</source>
      <translation>互いに無効となるグルーブの角度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="193"/>
      <location filename="../../../App/FeatureGroove.cpp" line="171"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>回転押し出しの軸がスケッチと交差しています。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="294"/>
      <location filename="../../../App/FeatureGroove.cpp" line="263"/>
      <source>Could not revolve the sketch!</source>
      <translation>スケッチを回転押し出しできませんでした!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="306"/>
      <location filename="../../../App/FeatureGroove.cpp" line="275"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>スケッチから面を作成できませんでした。
スケッチ内のスケッチ図形を交差させることはできません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="235"/>
      <source>Error: Pitch too small!</source>
      <translation>エラー：ピッチが小さすぎます！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="240"/>
      <location filename="../../../App/FeatureHelix.cpp" line="263"/>
      <source>Error: height too small!</source>
      <translation>エラー：高さが小さすぎます！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="249"/>
      <source>Error: pitch too small!</source>
      <translation>エラー：ピッチが小さすぎます！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="254"/>
      <location filename="../../../App/FeatureHelix.cpp" line="268"/>
      <location filename="../../../App/FeatureHelix.cpp" line="277"/>
      <source>Error: turns too small!</source>
      <translation>エラー: 巻数が小さすぎます！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="283"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>エラー: 高さと増加率のいずれかをゼロにすることはできません！</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="301"/>
      <source>Error: unsupported mode</source>
      <translation>エラー: サポートされていないモードです。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="315"/>
      <source>Error: No valid sketch or face</source>
      <translation>エラー: 有効なスケッチ、面がありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="328"/>
      <source>Error: Face must be planar</source>
      <translation>エラー: 面は平面でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2422"/>
      <location filename="../../../App/FeatureHelix.cpp" line="443"/>
      <location filename="../../../App/FeatureHelix.cpp" line="484"/>
      <source>Error: Result is not a solid</source>
      <translation>エラー: 結果はソリッドではありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="413"/>
      <source>Error: There is nothing to subtract</source>
      <translation>エラー: 減算の対象がありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="419"/>
      <location filename="../../../App/FeatureHelix.cpp" line="449"/>
      <location filename="../../../App/FeatureHelix.cpp" line="490"/>
      <source>Error: Result has multiple solids</source>
      <translation>エラー: 結果に複数のソリッドが存在します。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="434"/>
      <source>Error: Adding the helix failed</source>
      <translation>エラー: らせんの加算に失敗しました。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="466"/>
      <source>Error: Intersecting the helix failed</source>
      <translation>エラー: らせんの交差に失敗しました。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="475"/>
      <source>Error: Subtracting the helix failed</source>
      <translation>エラー: らせんの減算に失敗しました。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="506"/>
      <source>Error: Could not create face from sketch</source>
      <translation>エラー: スケッチから面を作成できませんでした。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1224"/>
      <source>Thread type is invalid</source>
      <translation>ねじ山の種類が無効です</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1764"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>ホールエラー: サポートされていない長さ指定です。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1770"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>ホールエラー: 穴の深さが正しくありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1796"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>ホールエラー: テーパー角度が正しくありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1820"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>ホールエラー: ホールカット直径が小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1825"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>ホールエラー: ホールカット深さは穴の深さより小さくなければなりません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1832"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>ホールエラー: ホールカットの深さはゼロ以上でなければなりません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1862"/>
      <source>Hole error: Invalid countersink</source>
      <translation>ホールエラー: 皿穴が正しくありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1898"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>ホールエラー: 錐先角度が正しくありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1915"/>
      <source>Hole error: Invalid drill point</source>
      <translation>ホールエラー: 錐先が正しくありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1952"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>ホールエラー: スケッチを回転できませんでした。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1959"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>ホールエラー: 結果シェイプが空です。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1972"/>
      <source>Error: Adding the thread failed</source>
      <translation>エラー: ねじ山の追加に失敗しました。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1983"/>
      <source>Hole error: Finding axis failed</source>
      <translation>ホールエラー: 軸が見つかりません</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2039"/>
      <location filename="../../../App/FeatureHole.cpp" line="2047"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation>プロファイルエッジでブーリアン演算に失敗</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2054"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation>ブーリアン演算によってプロファイルエッジ上で非ソリッドが生成</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="151"/>
      <source>Boolean operation failed</source>
      <translation>ブーリアン演算が失敗しました。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2080"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>スケッチから面を作成できませんでした。
スケッチ内の交差するスケッチ図形や複数面では、面までのポケットは作れません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2245"/>
      <source>Thread type out of range</source>
      <translation>ねじ山の種類が範囲外</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2248"/>
      <source>Thread size out of range</source>
      <translation>ねじ山のサイズが範囲外</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2396"/>
      <source>Error: Thread could not be built</source>
      <translation>エラー: ねじ山を作成できませんでした。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="191"/>
      <source>Loft: At least one section is needed</source>
      <translation>ロフト: 少なくとも1つのセクションが必要です。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="392"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>ロフト: ロフト作成中に重大なエラーが発生しました。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="238"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation>ロフト: スケッチから面を作成できませんでした。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="301"/>
      <location filename="../../../App/FeaturePipe.cpp" line="444"/>
      <source>Loft: Failed to create shell</source>
      <translation>ロフト: シェルの作成に失敗しました。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="817"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>スケッチから面を作成できませんでした。
スケッチ内のスケッチ図形や複数面を交差させることはできません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="203"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>パイプ: プロファイル形状を取得できませんでした。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="210"/>
      <source>No spine linked</source>
      <translation>スパインがリンクされていません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="225"/>
      <source>No auxiliary spine linked.</source>
      <translation>補助スパインがリンクされていません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="248"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>パイプ: セクションに孤立点を持つスケッチを使用する場合、孤立点が1つだけである必要があります。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="257"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>パイプ: プロファイルに1点を使用する場合、少なくとも1つのセクションが必要です。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="275"/>
      <source>Pipe: All sections need to be Part features</source>
      <translation>パイプ: すべてのセクションがパートフィーチャーである必要があります。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="283"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>パイプ: セクション形状を取得できませんでした。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="293"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>パイプ: プロファイルと最後のセクションにだけ頂点を配置できます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="306"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>複数セクションは、基本セクションと同じ量の内部ワイヤーを持つ必要があります。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="339"/>
      <source>Path must not be a null shape</source>
      <translation>パスは null シェイプであってはなりません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="379"/>
      <source>Pipe could not be built</source>
      <translation>パイプを作成できませんでした。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="436"/>
      <source>Result is not a solid</source>
      <translation>結果はソリッドではありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="475"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>パイプ: 次のものから減算する対象がありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="543"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>パイプ作成中に重大なエラーが発生しました。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="672"/>
      <source>Invalid element in spine.</source>
      <translation>スパインの要素が正しくありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="677"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>スパインの要素がエッジでもワイヤーでもありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="698"/>
      <source>Spine is not connected.</source>
      <translation>スパインが接続されていません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="704"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>スパインがエッジでもワイヤーでもありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="709"/>
      <source>Invalid spine.</source>
      <translation>スパインが正しくありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="101"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>ベースフィーチャー無しでプリミティブフィーチャーを減算することはできません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="353"/>
      <location filename="../../../App/FeaturePipe.cpp" line="505"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="123"/>
      <source>Unknown operation type</source>
      <translation>未知のオペレーション・タイプです。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="361"/>
      <location filename="../../../App/FeaturePipe.cpp" line="513"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="131"/>
      <source>Failed to perform boolean operation</source>
      <translation>ブール演算の実行に失敗しました。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="215"/>
      <source>Length of box too small</source>
      <translation>直方体の厚みが小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="220"/>
      <source>Width of box too small</source>
      <translation>直方体の幅が小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="225"/>
      <source>Height of box too small</source>
      <translation>直方体の高さが小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="273"/>
      <source>Radius of cylinder too small</source>
      <translation>円筒の半径が小さすぎます</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="278"/>
      <source>Height of cylinder too small</source>
      <translation>円筒の高さが小さすぎます</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="283"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>円筒の回転角が小さすぎます</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="340"/>
      <source>Radius of sphere too small</source>
      <translation>球の半径が小さすぎます</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="392"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="397"/>
      <source>Radius of cone cannot be negative</source>
      <translation>円錐の半径を負の値にすることはできません</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="402"/>
      <source>Height of cone too small</source>
      <translation>円錐の高さが小さすぎます</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="482"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="487"/>
      <source>Radius of ellipsoid too small</source>
      <translation>楕円体の半径が小さすぎます</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="581"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="586"/>
      <source>Radius of torus too small</source>
      <translation>トーラスの半径が小さすぎます</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="671"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>角柱を作る多角形は3つ以上の辺を持たなければならず、正しくありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="676"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>角柱を作る多角形の外接円が小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="681"/>
      <source>Height of prism is too small</source>
      <translation>角柱の高さが小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="768"/>
      <source>delta x of wedge too small</source>
      <translation>ウェッジのΔxが小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="774"/>
      <source>delta y of wedge too small</source>
      <translation>ウェッジのΔyが小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="780"/>
      <source>delta z of wedge too small</source>
      <translation>ウェッジのΔzが小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="786"/>
      <source>delta z2 of wedge is negative</source>
      <translation>ウェッジのΔz2が小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="792"/>
      <source>delta x2 of wedge is negative</source>
      <translation>ウェッジのΔx2が小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="123"/>
      <source>Angle of revolution too large</source>
      <translation>回転押し出しの角度が大きすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="130"/>
      <source>Angle of revolution too small</source>
      <translation>回転押し出しの角度が小さすぎます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="137"/>
      <source>Angles of revolution nullify each other</source>
      <translation>互いに無効となる回押し出しの角度</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolution.cpp" line="168"/>
      <location filename="../../../App/FeatureGroove.cpp" line="146"/>
      <source>Reference axis is invalid</source>
      <translation>参照軸が無効です。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="756"/>
      <source>Fusion with base feature failed</source>
      <translation>ベースフィーチャーとの結合に失敗しました。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="99"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>変換フィーチャーのリンク・オブジェクトはパートオブジェクトではありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="106"/>
      <source>No originals linked to the transformed feature.</source>
      <translation>変換されたフィーチャーにリンク元がありません。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="346"/>
      <source>Cannot transform invalid support shape</source>
      <translation>無効なサポートシェイプを変換できません</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="397"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>加算/減算フィーチャーのシェイプが空です。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="388"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>加算および減算フィーチャーのみ変換できます。</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="107"/>
      <source>Invalid face reference</source>
      <translation>無効な面参照です。</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="60"/>
      <source>Involute Gear</source>
      <translation>インボリュート歯車</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="64"/>
      <source>Creates or edits the involute gear definition</source>
      <translation>インボリュート歯車定義を作成、または編集</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="63"/>
      <source>Sprocket</source>
      <translation>スプロケット</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="67"/>
      <source>Creates or edits the sprocket definition.</source>
      <translation>スプロケット定義を作成、または編集</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPreviewParameters</name>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="20"/>
      <source>Show final result</source>
      <translation>最終結果を表示</translation>
    </message>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="27"/>
      <source>Show preview overlay</source>
      <translation>プレビューオーバーレイを表示</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="48"/>
      <source>Preview</source>
      <translation>プレビュー</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="223"/>
      <source>Shaft Design Wizard</source>
      <translation>シャフト設計ウィザード</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="226"/>
      <source>Starts the shaft design wizard</source>
      <translation>シャフト設計ウィザードを開始</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::FeatureAddSub</name>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="84"/>
      <source>Failure while computing removed volume preview: %1</source>
      <translation>削除体積のプレビューの計算中に問題が起きました: %1</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="105"/>
      <source>Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</source>
      <translation>結果シェイプが空です。マテリアルが削除されていない、またはモデルに問題がある可能性があります。</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2644"/>
      <source>Create Datum</source>
      <translation>データムを作成</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2645"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>データムオブジェクトまたはローカル座標系を作成</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2679"/>
      <source>Create Datum</source>
      <translation>データムを作成</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2680"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>データムオブジェクトまたはローカル座標系を作成</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="215"/>
      <source>Creates an additive box by its width, height, and length</source>
      <translation>幅、高さ、厚みから加算直方体を作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="224"/>
      <source>Creates an additive cylinder by its radius, height, and angle</source>
      <translation>半径、高さ、角度から加算円柱を作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="233"/>
      <source>Creates an additive sphere by its radius and various angles</source>
      <translation>半径と複数の角度から加算球を作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="242"/>
      <source>Creates an additive cone</source>
      <translation>加算円錐を作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="248"/>
      <source>Creates an additive ellipsoid</source>
      <translation>加算楕円体を作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="254"/>
      <source>Creates an additive torus</source>
      <translation>加算トーラスを作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="260"/>
      <source>Creates an additive prism</source>
      <translation>加算角柱を作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="266"/>
      <source>Creates an additive wedge</source>
      <translation>加算ウェッジを作成</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="400"/>
      <source>Creates a subtractive box by its width, height and length</source>
      <translation>幅、高さ、厚みから減算直方体を作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="409"/>
      <source>Creates a subtractive cylinder by its radius, height and angle</source>
      <translation>半径、高さ、角度から減算円柱を作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="418"/>
      <source>Creates a subtractive sphere by its radius and various angles</source>
      <translation>半径と複数の角度から減算球を作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="427"/>
      <source>Creates a subtractive cone</source>
      <translation>減算円錐を作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="433"/>
      <source>Creates a subtractive ellipsoid</source>
      <translation>減算楕円体を作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="439"/>
      <source>Creates a subtractive torus</source>
      <translation>減算トーラスを作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="445"/>
      <source>Creates a subtractive prism</source>
      <translation>減算角柱を作成</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="451"/>
      <source>Creates a subtractive wedge</source>
      <translation>減算ウェッジを作成</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1007"/>
      <source>Attachment</source>
      <translation>アタッチメント</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="835"/>
      <source>Revolution Parameters</source>
      <translation>回転押し出しパラメーター</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgGrooveParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="845"/>
      <source>Groove Parameters</source>
      <translation>グルーブパラメーター</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="37"/>
      <source>Transformed Feature Messages</source>
      <translation>変換フィーチャーメッセージ</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="122"/>
      <source>Active Body</source>
      <translation>アクティブなボディー</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="43"/>
      <source>Chamfer Parameters</source>
      <translation>面取りパラメーター</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="113"/>
      <source>Datum Plane Parameters</source>
      <translation>データム平面パラメーター</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="118"/>
      <source>Datum Line Parameters</source>
      <translation>データム線パラメーター</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="123"/>
      <source>Datum Point Parameters</source>
      <translation>データム点パラメーター</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="128"/>
      <source>Local Coordinate System Parameters</source>
      <translation>ローカル座標系パラメーター</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="44"/>
      <source>Draft Parameters</source>
      <translation>抜き勾配パラメーター</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="43"/>
      <source>Fillet Parameters</source>
      <translation>フィレットパラメーター</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="40"/>
      <source>Linear Pattern Parameters</source>
      <translation>直線状パターンパラメーター</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="40"/>
      <source>Mirror Parameters</source>
      <translation>鏡像パラメーター</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="40"/>
      <source>Multi-Transform Parameters</source>
      <translation>マルチ変換パラメーター</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="40"/>
      <source>Polar Pattern Parameters</source>
      <translation>軸周状パターンパラメーター</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="40"/>
      <source>Scale Parameters</source>
      <translation>拡大縮小パラメーター</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="43"/>
      <source>Thickness Parameters</source>
      <translation>厚みパラメーター</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPatternParameters</name>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="130"/>
      <source>Direction 2</source>
      <translation>セクション 2</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="246"/>
      <source>Select a direction reference (edge, face, datum line)</source>
      <translation>方向参照（エッジ、面、データム線）を選択</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="332"/>
      <source>Invalid selection. Select an edge, planar face, or datum line.</source>
      <translation>無効な選択です。エッジ、平面、またはデータム線を選択してください。</translation>
    </message>
  </context>
</TS>
