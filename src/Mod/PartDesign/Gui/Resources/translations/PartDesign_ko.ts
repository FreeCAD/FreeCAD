<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ko" sourcelanguage="en">
  <context>
    <name>App::Property</name>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="82"/>
      <source>The center point of the helix' start; derived from the reference axis.</source>
      <translation>나선 시작점의 중심점입니다. 기준 축에서 파생됩니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="92"/>
      <source>The helix' direction; derived from the reference axis.</source>
      <translation>나선의 방향입니다. 기준 축에서 파생됩니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="99"/>
      <source>The reference axis of the helix.</source>
      <translation>나선의 기준 축입니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="106"/>
      <source>The helix input mode specifies which properties are set by the user.
Dependent properties are then calculated.</source>
      <translation>나선 입력 모드는 사용자가 직접 설정할 속성을 지정합니다.
종속 속성은 그에 따라 계산됩니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="118"/>
      <source>The axial distance between two turns.</source>
      <translation>두 회전 사이의 축 방향 거리입니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="125"/>
      <source>The height of the helix' path, not accounting for the extent of the profile.</source>
      <translation>프로파일 범위를 고려하지 않은 나선 경로의 높이입니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="135"/>
      <source>The number of turns in the helix.</source>
      <translation>나선의 회전 수입니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="143"/>
      <source>The angle of the cone that forms a hull around the helix.
Non-zero values turn the helix into a conical spiral.
Positive values make the radius grow, negative shrinks.</source>
      <translation>나선 주위에 볼록 껍질을 형성하는 원뿔의 각도입니다.
0이 아닌 값이면 나선이 원뿔형 나선이 됩니다.
양수는 반지름이 커지고, 음수는 작아집니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="156"/>
      <source>The growth of the helix' radius per turn.
Non-zero values turn the helix into a conical spiral.</source>
      <translation>회전당 나선 반지름의 증가량입니다.
0이 아닌 값이면 나선이 원뿔형 나선이 됩니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="167"/>
      <source>Sets the turning direction to left handed,
i.e. counter-clockwise when moving along its axis.</source>
      <translation>회전 방향을 왼손 방향으로 설정합니다.
즉, 축을 따라 이동할 때 반시계 방향입니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="178"/>
      <source>Determines whether the helix points in the opposite direction of the axis.</source>
      <translation>나선이 축의 반대 방향을 향하는지 결정합니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="188"/>
      <source>If set, the result will be the intersection of the profile and the preexisting body.</source>
      <translation>설정하면 결과는 프로파일과 기존 바디의 교집합이 됩니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="198"/>
      <source>If false, the tool will propose an initial value for the pitch based on the profile bounding box,
so that self intersection is avoided.</source>
      <translation>거짓이면 도구가 프로파일 바운딩 박스를 기준으로 피치의 초기값을 제안하여 자기 교차를 피합니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="210"/>
      <source>Fusion Tolerance for the Helix, increase if helical shape does not merge nicely with part.</source>
      <translation>나선의 융합 공차입니다. 나선 형상이 파트와 잘 합쳐지지 않으면 값을 늘리십시오.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="108"/>
      <source>Number of gear teeth</source>
      <translation>기어 잇수</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="120"/>
      <source>Pressure angle of gear teeth</source>
      <translation>기어 이의 압력각</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="114"/>
      <source>Module of the gear</source>
      <translation>기어 모듈</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="129"/>
      <source>True=2 curves with each 3 control points, False=1 curve with 4 control points.</source>
      <translation>참=각각 제어점 3개인 곡선 2개, 거짓=제어점 4개인 곡선 1개입니다.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="137"/>
      <source>True=external Gear, False=internal Gear</source>
      <translation>참=외접 기어, 거짓=내접 기어</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="146"/>
      <source>The height of the tooth from the pitch circle up to its tip, normalized by the module.</source>
      <translation>피치원에서 치첨까지의 높이를 모듈로 정규화한 값입니다.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="155"/>
      <source>The height of the tooth from the pitch circle down to its root, normalized by the module.</source>
      <translation>피치원에서 치근까지의 높이를 모듈로 정규화한 값입니다.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="164"/>
      <source>The radius of the fillet at the root of the tooth, normalized by the module.</source>
      <translation>치근 필렛의 반지름을 모듈로 정규화한 값입니다.</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="173"/>
      <source>The distance by which the reference profile is shifted outwards, normalized by the module.</source>
      <translation>기준 프로파일이 바깥쪽으로 이동한 거리를 모듈로 정규화한 값입니다.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1664"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1665"/>
      <source>Additive Helix</source>
      <translation>나선 추가</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1666"/>
      <source>Sweeps the selected sketch or profile along a helix and adds it to the body</source>
      <translation>선택한 스케치나 프로파일을 나선을 따라 스윕하여 바디에 추가합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1565"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1566"/>
      <source>Additive Loft</source>
      <translation>로프트 추가</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1567"/>
      <source>Lofts the selected sketch or profile along a path and adds it to the body</source>
      <translation>선택한 스케치나 프로파일을 경로를 따라 로프트하여 바디에 추가합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignAdditivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1465"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1466"/>
      <source>Additive Pipe</source>
      <translation>파이프 추가</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1467"/>
      <source>Sweeps the selected sketch or profile along a path and adds it to the body</source>
      <translation>선택한 스케치나 프로파일을 경로를 따라 스윕하여 바디에 추가합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBody</name>
    <message>
      <location filename="../../CommandBody.cpp" line="92"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="93"/>
      <source>New Body</source>
      <translation>새 바디</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="94"/>
      <source>Creates a new body and activates it</source>
      <translation>새 바디를 만들고 활성화합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignBoolean</name>
    <message>
      <location filename="../../Command.cpp" line="2580"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2581"/>
      <source>Boolean Operation</source>
      <translation>부울 연산</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2582"/>
      <source>Applies boolean operations with the selected objects and the active body</source>
      <translation>선택한 객체와 활성 바디에 부울 연산을 적용합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCS</name>
    <message>
      <location filename="../../Command.cpp" line="282"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="283"/>
      <source>Local Coordinate System</source>
      <translation>로컬 좌표계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="284"/>
      <source>Creates a new local coordinate system</source>
      <translation>새 로컬 좌표계를 생성합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignChamfer</name>
    <message>
      <location filename="../../Command.cpp" line="1991"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1992"/>
      <source>Chamfer</source>
      <translation>모따기</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1993"/>
      <source>Applies a chamfer to the selected edges or faces</source>
      <translation>선택한 에지나 면에 모따기를 적용합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignClone</name>
    <message>
      <location filename="../../Command.cpp" line="492"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="493"/>
      <source>Clone</source>
      <translation>복제</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="494"/>
      <source>Copies a solid object parametrically as the base feature of a new body</source>
      <translation>솔리드 객체를 새 바디의 기준 피처로 파라메트릭하게 복사합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDraft</name>
    <message>
      <location filename="../../Command.cpp" line="2020"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2021"/>
      <source>Draft</source>
      <translation>구배</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2022"/>
      <source>Applies a draft to the selected faces</source>
      <translation>선택한 면에 구배를 적용합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignDuplicateSelection</name>
    <message>
      <location filename="../../CommandBody.cpp" line="762"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="763"/>
      <source>Duplicate &amp;Object</source>
      <translation>객체 복제(&amp;O)</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="764"/>
      <source>Duplicates the selected object and adds it to the active body</source>
      <translation>선택한 객체를 복제하여 활성 바디에 추가합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignFillet</name>
    <message>
      <location filename="../../Command.cpp" line="1963"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1964"/>
      <source>Fillet</source>
      <translation>모깎기</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1965"/>
      <source>Applies a fillet to the selected edges or faces</source>
      <translation>선택한 에지나 면에 모깎기를 적용합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignGroove</name>
    <message>
      <location filename="../../Command.cpp" line="1395"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1396"/>
      <source>Groove</source>
      <translation>회전 홈파기</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1397"/>
      <source>Revolves the sketch or profile around a line or axis and removes it from the body</source>
      <translation>스케치 또는 윤곽을 선이나 축을 중심으로 회전하여 생긴 부분을 바디에서 제거합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignHole</name>
    <message>
      <location filename="../../Command.cpp" line="1288"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1289"/>
      <source>Hole</source>
      <translation>구멍</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1291"/>
      <source>Creates holes in the active body at the center points of circles or arcs of the selected sketch or profile</source>
      <translation>활성화된 바디에서 선택한 스케치 또는 윤곽의 원이나 호의 중심점에 구멍을 생성합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLine</name>
    <message>
      <location filename="../../Command.cpp" line="222"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="223"/>
      <source>Datum Line</source>
      <translation>기준선</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="224"/>
      <source>Creates a new datum line</source>
      <translation>새 기준선을 생성합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignLinearPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2275"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2276"/>
      <source>Linear Pattern</source>
      <translation>선형 패턴</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2277"/>
      <source>Duplicates the selected features or the active body in a linear pattern</source>
      <translation>바디의 일부 피처 또는 (모든 피처를 담은) 바디 자체를 선형 패턴으로 복제합니다. 그 결과는 여전히 원래의 바디 안에 담깁니다.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMigrate</name>
    <message>
      <location filename="../../CommandBody.cpp" line="392"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="393"/>
      <source>Migrate</source>
      <translation>이전하기</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="394"/>
      <source>Migrates the document to the modern Part Design workflow</source>
      <translation>문서를 현대적인 파트 디자인 작업 흐름으로 마이그레이션합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMirrored</name>
    <message>
      <location filename="../../Command.cpp" line="2218"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2219"/>
      <source>Mirror</source>
      <translation>대칭 복사</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2220"/>
      <source>Mirrors the selected features or active body</source>
      <translation>선택한 피처 또는 활성 바디를 대칭 복사합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="830"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="831"/>
      <source>Move Object To…</source>
      <translation>객체 이동…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="832"/>
      <source>Moves the selected object to another body</source>
      <translation>선택한 객체를 다른 바디로 이동합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1027"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1028"/>
      <source>Move Feature After…</source>
      <translation>피처를 다음으로 이동…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1029"/>
      <source>Moves the selected feature after another feature in the same body</source>
      <translation>선택한 피처를 같은 바디의 다른 피처 뒤로 이동합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMoveTip</name>
    <message>
      <location filename="../../CommandBody.cpp" line="663"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="664"/>
      <source>Set Tip</source>
      <translation>팁 설정</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="665"/>
      <source>Moves the tip of the body to the selected feature</source>
      <translation>바디의 팁을 선택한 피처로 이동합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignMultiTransform</name>
    <message>
      <location filename="../../Command.cpp" line="2449"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2450"/>
      <source>Multi-Transform</source>
      <translation>다중 변환</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2451"/>
      <source>Applies multiple transformations to the selected features or active body</source>
      <translation>선택한 피처 또는 활성 바디에 여러 변환을 적용합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignNewSketch</name>
    <message>
      <location filename="../../Command.cpp" line="577"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="578"/>
      <source>New Sketch</source>
      <translation>새 스케치</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="579"/>
      <source>Creates a new sketch</source>
      <translation>새 스케치를 생성합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPad</name>
    <message>
      <location filename="../../Command.cpp" line="1230"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1231"/>
      <source>Pad</source>
      <translation>패드</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1232"/>
      <source>Extrudes the selected sketch or profile and adds it to the body</source>
      <translation>선택한 스케치나 윤곽을 돌출하여 생긴 부분을 바디에 추가합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPlane</name>
    <message>
      <location filename="../../Command.cpp" line="192"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="193"/>
      <source>Datum Plane</source>
      <translation>기준 평면</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="194"/>
      <source>Creates a new datum plane</source>
      <translation>새 기준 평면을 생성합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPocket</name>
    <message>
      <location filename="../../Command.cpp" line="1259"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1260"/>
      <source>Pocket</source>
      <translation>포켓</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1261"/>
      <source>Extrudes the selected sketch or profile and removes it from the body</source>
      <translation>선택한 스케치나 윤곽을 돌출하여 생긴 부분을 바디에서 제거합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPoint</name>
    <message>
      <location filename="../../Command.cpp" line="252"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="253"/>
      <source>Datum Point</source>
      <translation>기준점</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="254"/>
      <source>Creates a new datum point</source>
      <translation>새 기준점을 생성합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignPolarPattern</name>
    <message>
      <location filename="../../Command.cpp" line="2344"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2345"/>
      <source>Polar Pattern</source>
      <translation>원형 패턴</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2346"/>
      <source>Duplicates the selected features or the active body in a circular pattern</source>
      <translation>바디의 일부 피처 또는 (모든 피처를 담은) 바디 자체를 원형 패턴으로 복제합니다. 그 결과는 여전히 원래의 바디 안에 담깁니다.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignRevolution</name>
    <message>
      <location filename="../../Command.cpp" line="1333"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1334"/>
      <source>Revolve</source>
      <translation>회전</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1335"/>
      <source>Revolves the selected sketch or profile around a line or axis and adds it to the body</source>
      <translation>스케치 또는 윤곽을 선이나 축을 중심으로 회전하여 생긴 부분을 바디에 추가합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignScaled</name>
    <message>
      <location filename="../../Command.cpp" line="2406"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2407"/>
      <source>Scale</source>
      <translation>크기 조정</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2408"/>
      <source>Scales the selected features or the active body</source>
      <translation>선택한 피처 또는 활성 바디의 크기를 조정합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="316"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="317"/>
      <source>Shape Binder</source>
      <translation>형상 바인더</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="318"/>
      <source>Creates a new shape binder</source>
      <translation>새 형상 바인더를 생성합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubShapeBinder</name>
    <message>
      <location filename="../../Command.cpp" line="386"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="387"/>
      <source>Sub-Shape Binder</source>
      <translation>하위 형상 바인더</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="388"/>
      <source>Creates a reference to geometry from one or more objects, allowing it to be used inside or outside a body. It tracks relative placements, supports multiple geometry types (solids, faces, edges, vertices), and can work with objects in the same or external documents.</source>
      <translation>하나 이상의 객체에서 기하 형상 참조를 생성하여 바디 안이나 밖에서 사용할 수 있게 합니다. 상대 배치를 추적하고, 여러 기하 형상 유형(솔리드, 면, 에지, 꼭짓점)을 지원하며, 같은 문서나 외부 문서의 객체와 함께 사용할 수 있습니다.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveHelix</name>
    <message>
      <location filename="../../Command.cpp" line="1748"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1749"/>
      <source>Subtractive Helix</source>
      <translation>나선 제거</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1750"/>
      <source>Sweeps the selected sketch or profile along a helix and removes it from the body</source>
      <translation>선택한 스케치나 프로파일을 나선을 따라 스윕하여 바디에서 제거합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractiveLoft</name>
    <message>
      <location filename="../../Command.cpp" line="1615"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1616"/>
      <source>Subtractive Loft</source>
      <translation>로프트 제거</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1617"/>
      <source>Lofts the selected sketch or profile along a path and removes it from the body</source>
      <translation>선택한 스케치나 프로파일을 경로를 따라 로프트하여 바디에서 제거합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignSubtractivePipe</name>
    <message>
      <location filename="../../Command.cpp" line="1515"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1516"/>
      <source>Subtractive Pipe</source>
      <translation>파이프 제거</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1517"/>
      <source>Sweeps the selected sketch or profile along a path and removes it from the body</source>
      <translation>선택한 스케치나 프로파일을 경로를 따라 스윕하여 바디에서 제거합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignThickness</name>
    <message>
      <location filename="../../Command.cpp" line="2090"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2091"/>
      <source>Thickness</source>
      <translation>두께</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2092"/>
      <source>Applies thickness and removes the selected faces</source>
      <translation>두께를 적용하고 선택한 면을 제거합니다</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="76"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="77"/>
      <source>Additive Primitive</source>
      <translation>덧셈 기본도형</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="78"/>
      <source>Creates an additive primitive</source>
      <translation>덧셈 기본도형 생성</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="215"/>
      <source>Additive Box</source>
      <translation>박스 추가</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="224"/>
      <source>Additive Cylinder</source>
      <translation>실린더 추가</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="233"/>
      <source>Additive Sphere</source>
      <translation>구체 추가</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="242"/>
      <source>Additive Cone</source>
      <translation>원뿔 추가</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="248"/>
      <source>Additive Ellipsoid</source>
      <translation>타원체 추가</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="254"/>
      <source>Additive Torus</source>
      <translation>원환체 추가</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="260"/>
      <source>Additive Prism</source>
      <translation>각기둥 추가</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="266"/>
      <source>Additive Wedge</source>
      <translation>쐐기(wedge) 추가</translation>
    </message>
  </context>
  <context>
    <name>CmdPrimtiveCompSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="284"/>
      <source>PartDesign</source>
      <translation>단품설계</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="285"/>
      <source>Subtractive Primitive</source>
      <translation>뺄셈 기본도형</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="286"/>
      <source>Creates a subtractive primitive</source>
      <translation>뺄셈 기본도형 생성</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="400"/>
      <source>Subtractive Box</source>
      <translation>박스 잘라내기</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="409"/>
      <source>Subtractive Cylinder</source>
      <translation>실린더 잘라내기</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="418"/>
      <source>Subtractive Sphere</source>
      <translation>구체 잘라내기</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="427"/>
      <source>Subtractive Cone</source>
      <translation>원뿔 잘라내기</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="433"/>
      <source>Subtractive Ellipsoid</source>
      <translation>타원체 잘라내기</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="439"/>
      <source>Subtractive Torus</source>
      <translation>원환체 잘라내기</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="445"/>
      <source>Subtractive Prism</source>
      <translation>각기둥 잘라내기</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="451"/>
      <source>Subtractive Wedge</source>
      <translation>쐐기 잘라내기</translation>
    </message>
  </context>
  <context>
    <name>Command</name>
    <message>
      <location filename="../../Command.cpp" line="338"/>
      <source>Edit Shape Binder</source>
      <translation>형상 바인더 편집</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="349"/>
      <source>Create Shape Binder</source>
      <translation>형상 바인더 생성</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="442"/>
      <source>Create Sub-Shape Binder</source>
      <translation>하위 형상 바인더 생성</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="512"/>
      <source>Create Clone</source>
      <translation>복제하기</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1114"/>
      <source>Make Copy</source>
      <translation>사본 만들기</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2504"/>
      <source>Convert to Multi-Transform feature</source>
      <translation>다중 변환 피처로 변환</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="255"/>
      <source>Sketch on Face</source>
      <translation>면에 스케치</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="316"/>
      <source>Make copy</source>
      <translation>사본 만들기</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="518"/>
      <location filename="../../SketchWorkflow.cpp" line="775"/>
      <source>New Sketch</source>
      <translation>새 스케치</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2601"/>
      <source>Create Boolean</source>
      <translation>부울 생성</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="224"/>
      <location filename="../../DlgActiveBody.cpp" line="102"/>
      <source>Add a Body</source>
      <translation>몸통 추가</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="529"/>
      <source>Migrate legacy Part Design features to bodies</source>
      <translation>레거시 파트 디자인 피처를 바디로 마이그레이션</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="776"/>
      <source>Duplicate a Part Design object</source>
      <translation>파트 디자인 객체 복제</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1121"/>
      <source>Move a feature inside body</source>
      <translation>피처를 바디 안으로 이동</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="730"/>
      <source>Move tip to selected feature</source>
      <translation>선택한 피처로 팁 이동</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="935"/>
      <source>Move an object</source>
      <translation>객체 이동</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="261"/>
      <source>Mirror</source>
      <translation>대칭복사</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="301"/>
      <source>Linear Pattern</source>
      <translation>선형 패턴</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="350"/>
      <source>Polar Pattern</source>
      <translation>원형 패턴</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="389"/>
      <source>Scale</source>
      <translation>배율</translation>
    </message>
  </context>
  <context>
    <name>Gui::TaskView::TaskWatcherCommands</name>
    <message>
      <location filename="../../Workbench.cpp" line="55"/>
      <source>Face Tools</source>
      <translation>면 도구</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="56"/>
      <source>Edge Tools</source>
      <translation>에지 도구</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="57"/>
      <source>Boolean Tools</source>
      <translation>부울 연산 도구</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="58"/>
      <source>Helper Tools</source>
      <translation>도우미 도구</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="59"/>
      <source>Modeling Tools</source>
      <translation>조형 도구</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="60"/>
      <source>Create Geometry</source>
      <translation>도형 생성</translation>
    </message>
  </context>
  <context>
    <name>InvoluteGearParameter</name>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="14"/>
      <source>Involute Parameter</source>
      <translation>인벌류트 매개변수</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="20"/>
      <source>Number of teeth</source>
      <translation>잇수</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="40"/>
      <source>Module</source>
      <translation>모듈</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="84"/>
      <source>Pressure angle</source>
      <translation>압력각</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="125"/>
      <source>High precision</source>
      <translation>높은 정밀도</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="139"/>
      <location filename="../../../InvoluteGearFeature.ui" line="166"/>
      <source>True</source>
      <translation>참</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="144"/>
      <location filename="../../../InvoluteGearFeature.ui" line="171"/>
      <source>False</source>
      <translation>거짓</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="152"/>
      <source>External gear</source>
      <translation>외접 기어</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="179"/>
      <source>Addendum coefficient</source>
      <translation>치첨 높이 계수</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="199"/>
      <source>Dedendum coefficient</source>
      <translation>치근 높이 계수</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="219"/>
      <source>Root fillet coefficient</source>
      <translation>치근 필렛 계수</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.ui" line="239"/>
      <source>Profile shift coefficient</source>
      <translation>전위 계수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgActiveBody</name>
    <message>
      <location filename="../../DlgActiveBody.ui" line="14"/>
      <source>Active Body Required</source>
      <translation>활성 바디 필요</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="20"/>
      <source>To create a new Part Design object, there must be an active body in the document.
Select a body from below, or create a new body.</source>
      <translation>새 파트 디자인 객체를 만들려면 문서에 활성 바디가 있어야 합니다.
아래에서 바디를 선택하거나 새 바디를 만드십시오.</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.ui" line="35"/>
      <source>Create New Body</source>
      <translation>새 바디 만들기</translation>
    </message>
    <message>
      <location filename="../../DlgActiveBody.cpp" line="53"/>
      <source>Please select</source>
      <translation>선택하세요</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="14"/>
      <source>Geometric Primitives</source>
      <translation>기하학적 기본 도형</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="307"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="314"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1274"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1281"/>
      <source>Angle in first direction</source>
      <translation>첫 번째 방향의 각도</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="333"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="340"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1300"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1307"/>
      <source>Angle in second direction</source>
      <translation>두 번째 방향의 각도</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="62"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="153"/>
      <source>Length</source>
      <translation>길이:</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="82"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="173"/>
      <source>Width</source>
      <translation>너비</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="193"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="287"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="505"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1254"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1580"/>
      <source>Height</source>
      <translation>높이</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="267"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="625"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1600"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1749"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1805"/>
      <source>Radius</source>
      <translation>반지름</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="391"/>
      <source>Rotation angle</source>
      <translation>회전 각도</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="465"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="797"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1016"/>
      <source>Radius 1</source>
      <translation>반지름 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="485"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="820"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1039"/>
      <source>Radius 2</source>
      <translation>반지름 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="551"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1620"/>
      <source>Angle</source>
      <translation>각</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="674"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="896"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1091"/>
      <source>U parameter</source>
      <translation>U 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="694"/>
      <source>V parameters</source>
      <translation>V 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="804"/>
      <source>Radius in local z-direction</source>
      <translation>로컬 Z 방향의 반지름</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="827"/>
      <source>Radius in local X-direction</source>
      <translation>로컬 X 방향의 반지름</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="843"/>
      <source>Radius 3</source>
      <translation>반지름 3</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="850"/>
      <source>Radius in local Y-direction
If zero, it is equal to Radius2</source>
      <translation>로컬 Y 방향의 반지름
0이면 반지름 2와 같습니다</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="916"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1111"/>
      <source>V parameter</source>
      <translation>V 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1023"/>
      <source>Radius in local XY-plane</source>
      <translation>로컬 XY 평면에서의 반지름</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1046"/>
      <source>Radius in local XZ-plane</source>
      <translation>로컬 XZ 평면에서의 반지름</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1214"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2290"/>
      <source>Polygon</source>
      <translation>다각형</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1234"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="2313"/>
      <source>Circumradius</source>
      <translation>둘레</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1353"/>
      <source>X min/max</source>
      <translation>X 최소/최대</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1383"/>
      <source>Y min/max</source>
      <translation>Y 최소/최대</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1413"/>
      <source>Z min/max</source>
      <translation>Z 최소/최대</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1443"/>
      <source>X2 min/max</source>
      <translation>X2 최소/최대</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1476"/>
      <source>Z2 min/max</source>
      <translation>Z2 최소/최대</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1560"/>
      <source>Pitch</source>
      <translation>피치</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1637"/>
      <source>Coordinate system</source>
      <translation>좌표계</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1709"/>
      <source>Growth</source>
      <translation>증가량</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1729"/>
      <source>Number of rotations</source>
      <translation>회전 횟수</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1825"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1947"/>
      <source>Angle 1</source>
      <translation>각도 1</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1842"/>
      <location filename="../../TaskPrimitiveParameters.ui" line="1964"/>
      <source>Angle 2</source>
      <translation>각도 2</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1879"/>
      <source>From 3 Points</source>
      <translation>세 점에서</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1907"/>
      <source>Major radius</source>
      <translation>주반지름</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1927"/>
      <source>Minor radius</source>
      <translation>부반지름</translation>
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
      <translation>오른손</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="1650"/>
      <source>Left-handed</source>
      <translation>왼손</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2086"/>
      <source>Start point</source>
      <translation>시작 점</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.ui" line="2160"/>
      <source>End point</source>
      <translation>끝점</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::DlgReference</name>
    <message>
      <location filename="../../DlgReference.ui" line="14"/>
      <source>Reference</source>
      <translation>참조</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="20"/>
      <source>You selected geometries which are not part of the active body. Please define how to handle those selections. If you do not want those references, cancel the command.</source>
      <translation>선택한 기하 형상은 활성 바디의 일부가 아닙니다. 이 선택을 어떻게 처리할지 지정하십시오. 이러한 참조를 원하지 않으면 명령을 취소하십시오.</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="42"/>
      <source>Make independent copy (recommended)</source>
      <translation>독립 사본 만들기(권장)</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="52"/>
      <source>Make dependent copy</source>
      <translation>종속 사본 만들기</translation>
    </message>
    <message>
      <location filename="../../DlgReference.ui" line="59"/>
      <source>Create cross-reference</source>
      <translation>교차 참조 생성</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::NoDependentsSelection</name>
    <message>
      <location filename="../../ReferenceSelection.cpp" line="287"/>
      <source>Selecting this will cause circular dependency.</source>
      <translation>이 항목을 선택하면 순환 종속성이 발생합니다.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="22"/>
      <source>Add Body</source>
      <translation>바디 추가</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="32"/>
      <source>Remove Body</source>
      <translation>바디 제거</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="48"/>
      <source>Fuse</source>
      <translation>합집합</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="53"/>
      <source>Cut</source>
      <translation>자르기</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.ui" line="58"/>
      <source>Common</source>
      <translation>교집합</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="53"/>
      <source>Boolean Parameters</source>
      <translation>부울 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="84"/>
      <source>Remove</source>
      <translation>제거</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskBoxPrimitives</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="52"/>
      <source>Primitive Parameters</source>
      <translation>프리미티브 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="944"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="952"/>
      <location filename="../../TaskPrimitiveParameters.cpp" line="960"/>
      <source>Invalid wedge parameters</source>
      <translation>잘못된 쐐기 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="945"/>
      <source>X min must not be equal to X max!</source>
      <translation>최소 X값은 최대 X값과 같아서는 안 됩니다</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="953"/>
      <source>Y min must not be equal to Y max!</source>
      <translation>최소 Y값은 최대 Y값과 같아서는 안 됩니다</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="961"/>
      <source>Z min must not be equal to Z max!</source>
      <translation>최소 Z값은 최대 Z값과 같아서는 안 됩니다.</translation>
    </message>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1003"/>
      <source>Create primitive</source>
      <translation>프리미티브 생성</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskChamferParameters</name>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>선택 모드와 미리 보기 모드를 전환합니다</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="23"/>
      <source>Select</source>
      <translation>선택</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the chamfers</source>
      <translation>- 강조 표시할 항목을 선택합니다
- 항목을 더블클릭하여 모따기 결과를 볼 수 있습니다</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="48"/>
      <source>Type</source>
      <translation>유형</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="56"/>
      <source>Equal distance</source>
      <translation>같은 거리</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="61"/>
      <source>Two distances</source>
      <translation>양쪽 거리</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="66"/>
      <source>Distance and angle</source>
      <translation>거리와 각도</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="79"/>
      <source>Flips the direction</source>
      <translation>방향을 반대로 뒤집습니다</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="119"/>
      <source>Use all edges</source>
      <translation>모든 에지 사용</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="100"/>
      <source>Size</source>
      <translation>크기</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="146"/>
      <source>Size 2</source>
      <translation>크기 2</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.ui" line="179"/>
      <source>Angle</source>
      <translation>각</translation>
    </message>
    <message>
      <location filename="../../TaskChamferParameters.cpp" line="346"/>
      <source>Empty chamfer created!
</source>
      <translation>모따기가 생성되지 않았습니다!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgBooleanParameters</name>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="386"/>
      <source>Empty body list</source>
      <translation>비어 있는 바디 목록</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="386"/>
      <source>The body list cannot be empty</source>
      <translation>바디 목록은 비어 있을 수 없습니다</translation>
    </message>
    <message>
      <location filename="../../TaskBooleanParameters.cpp" line="407"/>
      <source>Boolean: Accept: Input error</source>
      <translation>불리언: 수락: 입력 오류</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgDatumParameters</name>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="107"/>
      <source>Incompatible Reference Set</source>
      <translation>호환되지 않는 참조 집합</translation>
    </message>
    <message>
      <location filename="../../TaskDatumParameters.cpp" line="109"/>
      <source>There is no attachment mode that fits the current set of references. If you choose to continue, the feature will remain where it is now, and will not be moved as the references change. Continue?</source>
      <translation>현재 참조 집합에 맞는 부착 모드가 없습니다. 계속을 선택하면 피처는 현재 위치에 그대로 유지되며 참조가 바뀌어도 이동하지 않습니다. 계속하시겠습니까?</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="443"/>
      <source>Input error</source>
      <translation>입력 오류</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDraftParameters</name>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>선택 모드와 미리 보기 모드를 전환합니다</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="23"/>
      <source>Select</source>
      <translation>선택</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the drafts</source>
      <translation>- 강조할 항목을 선택합니다
- 항목을 더블클릭하면 구배를 볼 수 있습니다</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="46"/>
      <source>Draft angle</source>
      <translation>구배 각</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="79"/>
      <source>Neutral Plane</source>
      <translation>중립 평면</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="96"/>
      <source>Pull Direction</source>
      <translation>인출 방향</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.ui" line="111"/>
      <source>Reverse pull direction</source>
      <translation>인출 방향 반전</translation>
    </message>
    <message>
      <location filename="../../TaskDraftParameters.cpp" line="304"/>
      <source>Empty draft created!
</source>
      <translation>구배가 생성되지 않았습니다!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDressUpParameters</name>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="302"/>
      <source>Select</source>
      <translation>선택</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="307"/>
      <source>Confirm Selection</source>
      <translation>선택 확인</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="320"/>
      <source>Add All Edges</source>
      <translation>모든 에지 추가</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="326"/>
      <source>Adds all edges to the list box (only when in add selection mode)</source>
      <translation>모든 에지를 목록 상자에 추가합니다(선택 추가 모드에서만).</translation>
    </message>
    <message>
      <location filename="../../TaskDressUpParameters.cpp" line="335"/>
      <source>Remove</source>
      <translation>제거</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskExtrudeParameters</name>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1374"/>
      <source>No face selected</source>
      <translation>선택한 면이 없습니다</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="173"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="1143"/>
      <source>Face</source>
      <translation>면</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="77"/>
      <source>Remove</source>
      <translation>제거</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="354"/>
      <source>Preview</source>
      <translation>미리 보기</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="358"/>
      <source>Select Faces</source>
      <translation>면 선택</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="694"/>
      <source>Select reference…</source>
      <translation>참조 선택…</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="604"/>
      <source>No shape selected</source>
      <translation>선택한 형상이 없습니다</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="687"/>
      <source>Sketch normal</source>
      <translation>스케치 법선</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="690"/>
      <source>Face normal</source>
      <translation>면에 수직</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="698"/>
      <location filename="../../TaskExtrudeParameters.cpp" line="701"/>
      <source>Custom direction</source>
      <translation>사용자 정의 방향</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1090"/>
      <source>Click on a shape in the model</source>
      <translation>모형에서 형상을 클릭하세요</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1361"/>
      <source>One sided</source>
      <translation>한쪽</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1362"/>
      <source>Two sided</source>
      <translation>양쪽</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1363"/>
      <source>Symmetric</source>
      <translation>대칭</translation>
    </message>
    <message>
      <location filename="../../TaskExtrudeParameters.cpp" line="1369"/>
      <source>Click on a face in the model</source>
      <translation>모형의 면을 클릭하세요</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFeaturePick</name>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="23"/>
      <source>Allow used features</source>
      <translation>사용된 피처들 허용</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="30"/>
      <source>Allow External Features</source>
      <translation>외부 피처 허용</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="42"/>
      <source>From other bodies of the same part</source>
      <translation>같은 부품의 다른 바디에서</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="49"/>
      <source>From different parts or free features</source>
      <translation>다른 부품들이나 자유 피처들에서</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="66"/>
      <source>Make independent copy (recommended)</source>
      <translation>독립 사본 만들기(권장)</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="79"/>
      <source>Make dependent copy</source>
      <translation>종속 사본 만들기</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.ui" line="89"/>
      <source>Create cross-reference</source>
      <translation>교차 참조 생성</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="69"/>
      <source>Valid</source>
      <translation>유효한</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="71"/>
      <source>Invalid shape</source>
      <translation>유효하지 않은 형상</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="73"/>
      <source>No wire in sketch</source>
      <translation>스케치에 철사가 없습니다.</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="75"/>
      <source>Sketch already used by other feature</source>
      <translation>선택한 스케치가 이미 다른 도형특징에서 사용되고 있습니다.</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="77"/>
      <source>Belongs to another body</source>
      <translation>다른 바디에 속함</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="79"/>
      <source>Belongs to another part</source>
      <translation>다른 부품에 속해있는</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="81"/>
      <source>Doesn't belong to any body</source>
      <translation>어느 바디에도 속하지 않음</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="83"/>
      <source>Base plane</source>
      <translation>기본 평면</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="85"/>
      <source>Feature is located after the tip of the body</source>
      <translation>피처은 바디의 팁 특징 뒤에 배치됩니다</translation>
    </message>
    <message>
      <location filename="../../TaskFeaturePick.cpp" line="97"/>
      <source>Select Attachment</source>
      <translation type="unfinished">Select Attachment</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskFilletParameters</name>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>선택 모드와 미리 보기 모드를 전환합니다</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="23"/>
      <source>Select</source>
      <translation>선택</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the fillets</source>
      <translation>항목을 선택하여 강조표시를 하십시오
항목을 더블클릭하여 필렛을 확인하세요</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="46"/>
      <source>Radius</source>
      <translation>반지름</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.ui" line="62"/>
      <source>Use all edges</source>
      <translation>모든 에지 사용</translation>
    </message>
    <message>
      <location filename="../../TaskFilletParameters.cpp" line="205"/>
      <source>Empty fillet created!</source>
      <translation>빈 모깎기가 생성되었습니다!！</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHelixParameters</name>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="29"/>
      <source>Valid</source>
      <translation>유효한</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="48"/>
      <location filename="../../TaskHelixParameters.cpp" line="241"/>
      <source>Base X-axis</source>
      <translation>기준 X축</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="53"/>
      <location filename="../../TaskHelixParameters.cpp" line="242"/>
      <source>Base Y-axis</source>
      <translation>기준 Y축</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="58"/>
      <location filename="../../TaskHelixParameters.cpp" line="243"/>
      <source>Base Z-axis</source>
      <translation>기준 Z축</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="63"/>
      <location filename="../../TaskHelixParameters.cpp" line="225"/>
      <source>Horizontal sketch axis</source>
      <translation type="unfinished">Horizontal sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="68"/>
      <location filename="../../TaskHelixParameters.cpp" line="224"/>
      <source>Vertical sketch axis</source>
      <translation type="unfinished">Vertical sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="73"/>
      <location filename="../../TaskHelixParameters.cpp" line="223"/>
      <source>Normal sketch axis</source>
      <translation type="unfinished">Normal sketch axis</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="22"/>
      <source>Status</source>
      <translation>상태</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="40"/>
      <source>Axis</source>
      <translation>축</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="78"/>
      <location filename="../../TaskHelixParameters.cpp" line="208"/>
      <source>Select reference…</source>
      <translation>참조 선택…</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="90"/>
      <source>Mode</source>
      <translation>방식</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="98"/>
      <source>Pitch-Height-Angle</source>
      <translation>피치-높이-각도</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="103"/>
      <source>Pitch-Turns-Angle</source>
      <translation>피치-회전수-각도</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="108"/>
      <source>Height-Turns-Angle</source>
      <translation>높이-회전수-각도</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="113"/>
      <source>Height-Turns-Growth</source>
      <translation>높이-회전수-확장</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="125"/>
      <source>Pitch</source>
      <translation>피치</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="152"/>
      <source>Height</source>
      <translation>높이</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="179"/>
      <source>Turns</source>
      <translation>회전수</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="203"/>
      <source>Cone angle</source>
      <translation>원뿔 각도</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="233"/>
      <source>Radial growth</source>
      <translation>반경 증가량</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="289"/>
      <source>Recompute on change</source>
      <translation>변경시 재계산</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="255"/>
      <source>Left handed</source>
      <translation>왼손잡이</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="265"/>
      <source>Reversed</source>
      <translation type="unfinished">Reversed</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.ui" line="272"/>
      <source>Remove outside of profile</source>
      <translation>윤곽 외부 제거</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="57"/>
      <source>Helix Parameters</source>
      <translation>나선 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="227"/>
      <source>Construction line %1</source>
      <translation>보조선</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="295"/>
      <source>Warning: helix might be self intersecting</source>
      <translation>주의: 나선이 자기교차할 수 있습니다</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="300"/>
      <source>Error: helix touches itself</source>
      <translation>오류: 나선이 자기자신에 접촉했습니다</translation>
    </message>
    <message>
      <location filename="../../TaskHelixParameters.cpp" line="349"/>
      <source>Error: unsupported mode</source>
      <translation>오류: 지원되지 않는 모드</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskHoleParameters</name>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="57"/>
      <source>Counterbore</source>
      <translation>카운터보어</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="58"/>
      <source>Countersink</source>
      <translation>접시형 구멍</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="59"/>
      <source>Counterdrill</source>
      <translation>카운터드릴</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="63"/>
      <source>Hole Parameters</source>
      <translation>구멍 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="72"/>
      <source>None</source>
      <translation>없음</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="73"/>
      <source>ISO metric regular</source>
      <translation>ISO 미터 일반</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="74"/>
      <source>ISO metric fine</source>
      <translation>ISO 미터 미세</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="75"/>
      <source>UTS coarse</source>
      <translation>UTS 보통</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="76"/>
      <source>UTS fine</source>
      <translation>UTS 미세</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="77"/>
      <source>UTS extra fine</source>
      <translation>UTS 초미세</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="78"/>
      <source>ANSI pipes</source>
      <translation>ANSI 파이프</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="79"/>
      <source>ISO/BSP pipes</source>
      <translation>ISO/BSP 파이프</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="80"/>
      <source>BSW whitworth</source>
      <translation>BSW 휘트워스</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="81"/>
      <source>BSF whitworth fine</source>
      <translation>BSF 휘트워스 미세</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="82"/>
      <source>ISO tyre valves</source>
      <translation>ISO 타이어 밸브</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="712"/>
      <source>Medium</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>중간</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="716"/>
      <source>Fine</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>세밀한</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="720"/>
      <source>Coarse</source>
      <comment>Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible</comment>
      <translation>거친</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="726"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>일반</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="730"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>닫기</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="734"/>
      <source>Loose</source>
      <comment>Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible</comment>
      <translation>느슨함</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="738"/>
      <source>Normal</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>일반</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="739"/>
      <source>Close</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>닫기</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.cpp" line="740"/>
      <source>Wide</source>
      <comment>Distance between thread crest and hole wall</comment>
      <translation>넓다</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskLoftParameters</name>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="20"/>
      <source>Ruled surface</source>
      <translation>선직면</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="27"/>
      <source>Closed</source>
      <translation>닫힌</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="34"/>
      <source>Profile</source>
      <translation>윤곽</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="42"/>
      <source>Object</source>
      <translation>객체</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="65"/>
      <source>Add Section</source>
      <translation>단면 추가</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="78"/>
      <source>Remove Section</source>
      <translation>단면 제거</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="103"/>
      <source>List can be reordered by dragging</source>
      <translation>드래그하여 목록을 재정렬할 수 있습니다.</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.ui" line="120"/>
      <source>Recompute on change</source>
      <translation>변경시 재계산</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="50"/>
      <source>Loft Parameters</source>
      <translation>로프트 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskLoftParameters.cpp" line="74"/>
      <source>Remove</source>
      <translation>제거</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMirroredParameters</name>
    <message>
      <location filename="../../TaskMirroredParameters.ui" line="34"/>
      <source>Plane</source>
      <translation>평면</translation>
    </message>
    <message>
      <location filename="../../TaskMirroredParameters.cpp" line="186"/>
      <source>Error</source>
      <translation>오류</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskMultiTransformParameters</name>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="32"/>
      <source>Transformations</source>
      <translation>변환</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.ui" line="52"/>
      <source>OK</source>
      <translation>확인</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="71"/>
      <source>Edit</source>
      <translation>편집</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="74"/>
      <source>Delete</source>
      <translation>삭제</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="77"/>
      <source>Add Mirror Transformation</source>
      <translation>대칭 변환 추가</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="85"/>
      <source>Add Linear Pattern</source>
      <translation>선형 패턴 추가</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="93"/>
      <source>Add Polar Pattern</source>
      <translation>원형 패턴 추가</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="101"/>
      <source>Add Scale Transformation</source>
      <translation>크기 조정 변환 추가</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="104"/>
      <source>Move Up</source>
      <translation>위로 이동</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="107"/>
      <source>Move Down</source>
      <translation>아래로 이동</translation>
    </message>
    <message>
      <location filename="../../TaskMultiTransformParameters.cpp" line="139"/>
      <source>Right-click to add a transformation</source>
      <translation>변환을 추가하려면 마우스 오른쪽 버튼을 클릭하십시오</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadParameters</name>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="40"/>
      <source>Pad Parameters</source>
      <translation>패드 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="42"/>
      <source>Offset the pad from the face at which the pad will end on side 1</source>
      <translation>패드가 1번 쪽에서 끝날 면으로부터의 오프셋입니다</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="43"/>
      <source>Offset the pad from the face at which the pad will end on side 2</source>
      <translation>패드가 2번 쪽에서 끝날 면으로부터의 오프셋입니다</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="44"/>
      <source>Reverses pad direction</source>
      <translation>패드 생성 방향 반전</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="73"/>
      <source>Dimension</source>
      <translation>치수</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="74"/>
      <source>To last</source>
      <translation>끝까지</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="75"/>
      <source>To first</source>
      <translation>첫 번째 만나는 면까지</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="76"/>
      <source>Up to face</source>
      <translation>면 까지</translation>
    </message>
    <message>
      <location filename="../../TaskPadParameters.cpp" line="77"/>
      <source>Up to shape</source>
      <translation>형상까지</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPadPocketParameters</name>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="68"/>
      <location filename="../../TaskPadPocketParameters.ui" line="303"/>
      <source>Type</source>
      <translation>유형</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="76"/>
      <source>Dimension</source>
      <translation>치수</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="84"/>
      <location filename="../../TaskPadPocketParameters.ui" line="313"/>
      <source>Length</source>
      <translation>길이:</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="101"/>
      <location filename="../../TaskPadPocketParameters.ui" line="330"/>
      <source>Offset to face</source>
      <translation>면에서 편차</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="192"/>
      <location filename="../../TaskPadPocketParameters.ui" line="421"/>
      <source>Select all faces</source>
      <translation>모든 면 선택</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="217"/>
      <location filename="../../TaskPadPocketParameters.ui" line="446"/>
      <source>Select</source>
      <translation>선택</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="255"/>
      <location filename="../../TaskPadPocketParameters.ui" line="484"/>
      <source>Select Face</source>
      <translation>면 선택</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="281"/>
      <source>Side 2</source>
      <translation>측 2</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="512"/>
      <source>Direction</source>
      <translation>방향</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="541"/>
      <source>Set a direction or select an edge
from the model as reference</source>
      <translation>방향을 설정하거나 참조로 모형의 에지를 선택하시오</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="546"/>
      <source>Sketch normal</source>
      <translation>스케치 법선</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="556"/>
      <source>Custom direction</source>
      <translation>사용자 정의 방향</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="569"/>
      <source>Use custom vector for pad direction, otherwise
the sketch plane's normal vector will be used</source>
      <translation>패드의 생성방향으로 사용자정의 벡터을 사용하세요, 그렇지 않으면 스케치 평면에 법선 벡터이 사용될 것입니다</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="521"/>
      <source>If unchecked, the length will be
measured along the specified direction</source>
      <translation>체크를 해제하면, 지정된 방향을 따라 길이가 측정됩니다.</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="525"/>
      <source>Length along sketch normal</source>
      <translation>스케치의 법선을 따른 길이</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="214"/>
      <location filename="../../TaskPadPocketParameters.ui" line="443"/>
      <source>Toggles between selection and preview mode</source>
      <translation>선택 모드와 미리 보기 모드를 전환합니다</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="505"/>
      <source>Reversed</source>
      <translation>반전</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="534"/>
      <source>Direction/edge</source>
      <translation>방향/에지</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="551"/>
      <source>Select reference…</source>
      <translation>참조 선택…</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="582"/>
      <source>X</source>
      <translation>X</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="589"/>
      <source>X-component of direction vector</source>
      <translation>방향 벡터의 X 성분</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="611"/>
      <source>Y</source>
      <translation>Y</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="618"/>
      <source>Y-component of direction vector</source>
      <translation>방향 벡터의 Y 성분</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="640"/>
      <source>Z</source>
      <translation>Z</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="647"/>
      <source>Z-component of direction vector</source>
      <translation>방향 벡터의 Z 성분</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="118"/>
      <location filename="../../TaskPadPocketParameters.ui" line="347"/>
      <source>Angle to taper the extrusion</source>
      <translation>돌출 경사각</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="22"/>
      <source>Mode</source>
      <translation>모드</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="46"/>
      <source>Side 1</source>
      <translation>측 1</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="121"/>
      <location filename="../../TaskPadPocketParameters.ui" line="350"/>
      <source>Taper angle</source>
      <translation>경사각</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="174"/>
      <location filename="../../TaskPadPocketParameters.ui" line="403"/>
      <source>Select Shape</source>
      <translation>형상 선택</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="189"/>
      <location filename="../../TaskPadPocketParameters.ui" line="418"/>
      <source>Selects all faces of the shape</source>
      <translation>형상의 모든 면을 선택합니다</translation>
    </message>
    <message>
      <location filename="../../TaskPadPocketParameters.ui" line="678"/>
      <source>Recompute on change</source>
      <translation>변경시 재계산</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeOrientation</name>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="22"/>
      <source>Orientation mode</source>
      <translation>원점 모드</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="36"/>
      <source>Standard</source>
      <translation>표준</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="41"/>
      <source>Fixed</source>
      <translation>고정된</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="46"/>
      <source>Frenet</source>
      <translation>프레네</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="51"/>
      <source>Auxiliary</source>
      <translation>보조</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="56"/>
      <source>Binormal</source>
      <translation>종법선</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="76"/>
      <source>Curvilinear equivalence</source>
      <translation>곡선 길이 기준</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="83"/>
      <source>Profile</source>
      <translation>윤곽</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="91"/>
      <source>Object</source>
      <translation>대상체</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="125"/>
      <source>Add Edge</source>
      <translation>에지 추가</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="144"/>
      <source>Remove Edge</source>
      <translation>에지 제거</translation>
    </message>
    <message>
      <location filename="../../TaskPipeOrientation.ui" line="166"/>
      <source>Set the constant binormal vector used to calculate the profiles orientation</source>
      <translation>윤곽의의 방향을 계산하는 데 사용되는 상수 종법선 벡터을 설정하십시오.</translation>
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
      <location filename="../../TaskPipeParameters.cpp" line="598"/>
      <source>Section Orientation</source>
      <translation>단면 방향</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="626"/>
      <source>Remove</source>
      <translation>제거</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeParameters</name>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="20"/>
      <source>Profile</source>
      <translation>윤곽</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="28"/>
      <location filename="../../TaskPipeParameters.ui" line="93"/>
      <source>Object</source>
      <translation>대상체</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="51"/>
      <source>Corner transition</source>
      <translation>코너 전이</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="70"/>
      <source>Right corner</source>
      <translation>각진 모퉁이</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="75"/>
      <source>Round corner</source>
      <translation>둥근 모퉁이</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="85"/>
      <source>Path to Sweep Along</source>
      <translation>쓸어 나가는 경로</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="119"/>
      <source>Add edge</source>
      <translation>에지 추가</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="138"/>
      <source>Remove edge</source>
      <translation>에지 제거</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.ui" line="65"/>
      <source>Transformed</source>
      <translation>변경된</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="69"/>
      <source>Pipe Parameters</source>
      <translation>파이프 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="80"/>
      <source>Select All</source>
      <translation>모두 선택</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="98"/>
      <source>Remove</source>
      <translation>제거</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="463"/>
      <location filename="../../TaskPipeParameters.cpp" line="584"/>
      <source>Input error</source>
      <translation>입력 오류</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="463"/>
      <source>No active body</source>
      <translation>활성화된 몸통 없음</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPipeScaling</name>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="22"/>
      <source>Transform mode</source>
      <translation>변형 모드</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="36"/>
      <source>Constant</source>
      <translation>상수</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="41"/>
      <source>Multisection</source>
      <translation>다중 단면</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="64"/>
      <source>Add Section</source>
      <translation>단면 추가</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="77"/>
      <source>Remove Section</source>
      <translation>단면 제거</translation>
    </message>
    <message>
      <location filename="../../TaskPipeScaling.ui" line="102"/>
      <source>List can be reordered by dragging</source>
      <translation>드래그하여 목록을 재정렬할 수 있습니다.</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="897"/>
      <source>Section Transformation</source>
      <translation>단면 변환</translation>
    </message>
    <message>
      <location filename="../../TaskPipeParameters.cpp" line="916"/>
      <source>Remove</source>
      <translation>제거</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPocketParameters</name>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="40"/>
      <source>Pocket Parameters</source>
      <translation>포켓 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="43"/>
      <source>Offset from the selected face at which the pocket will end on side 1</source>
      <translation>포켓이 1번 쪽에서 끝날 선택한 면으로부터의 오프셋입니다</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="46"/>
      <source>Offset from the selected face at which the pocket will end on side 2</source>
      <translation>포켓이 2번 쪽에서 끝날 선택한 면으로부터의 오프셋입니다</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="48"/>
      <source>Reverses pocket direction</source>
      <translation>홈파기 방향 반전</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="77"/>
      <source>Dimension</source>
      <translation>치수</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="78"/>
      <source>Through all</source>
      <translation>관통</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="79"/>
      <source>To first</source>
      <translation>첫 번째 만나는 면까지</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="80"/>
      <source>Up to face</source>
      <translation>면 까지</translation>
    </message>
    <message>
      <location filename="../../TaskPocketParameters.cpp" line="81"/>
      <source>Up to shape</source>
      <translation>형상까지</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="22"/>
      <source>Type</source>
      <translation>유형</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="50"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="222"/>
      <source>Base X-axis</source>
      <translation>기준 X축</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="55"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="223"/>
      <source>Base Y-axis</source>
      <translation>기준 Y축</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="60"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="224"/>
      <source>Base Z-axis</source>
      <translation>기준 Z축</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="65"/>
      <source>Horizontal sketch axis</source>
      <translation>수평 스케치 축</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="70"/>
      <source>Vertical sketch axis</source>
      <translation>수직 스케치 축</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="154"/>
      <source>Symmetric to plane</source>
      <translation>평면에 대칭</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="161"/>
      <source>Reversed</source>
      <translation>반전된</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="120"/>
      <source>2nd angle</source>
      <translation>두 번째 각도</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="42"/>
      <source>Axis</source>
      <translation>축</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="75"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="232"/>
      <source>Select reference…</source>
      <translation>참조 선택…</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="87"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="175"/>
      <source>Angle</source>
      <translation>각</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="170"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="149"/>
      <location filename="../../TaskRevolutionParameters.cpp" line="459"/>
      <source>Face</source>
      <translation>면</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.ui" line="192"/>
      <source>Recompute on change</source>
      <translation>변경시 재계산</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="177"/>
      <source>To last</source>
      <translation>끝까지</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="180"/>
      <source>Through all</source>
      <translation>관통</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="182"/>
      <source>To first</source>
      <translation>첫 번째 만나는 면까지</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="183"/>
      <source>Up to face</source>
      <translation>곡면까지</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="184"/>
      <source>Two angles</source>
      <translation>두 각도</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="447"/>
      <source>No face selected</source>
      <translation>선택한 면이 없습니다</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskScaledParameters</name>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="34"/>
      <source>Factor</source>
      <translation>비율</translation>
    </message>
    <message>
      <location filename="../../TaskScaledParameters.ui" line="48"/>
      <source>Occurrences</source>
      <translation>발생 횟수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskShapeBinder</name>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="22"/>
      <source>Object</source>
      <translation>대상체</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="48"/>
      <source>Add Geometry</source>
      <translation>도형 추가</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.ui" line="67"/>
      <source>Remove Geometry</source>
      <translation>도형 제거</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="61"/>
      <source>Shape Binder Parameters</source>
      <translation>형상 바인더 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskShapeBinder.cpp" line="139"/>
      <source>Remove</source>
      <translation>제거</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskSketchBasedParameters</name>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="204"/>
      <source>Face</source>
      <translation>면</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskThicknessParameters</name>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="20"/>
      <source>Toggles between selection and preview mode</source>
      <translation>선택 모드와 미리 보기 모드를 전환합니다</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="23"/>
      <source>Select</source>
      <translation>선택</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="33"/>
      <source>- select an item to highlight it
- double-click on an item to see the features</source>
      <translation>항목을 선택하여 강조표시를 하십시오
항목을 더블클릭하여 특징을 확인하세요</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="46"/>
      <source>Thickness</source>
      <translation>두께</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="75"/>
      <source>Mode</source>
      <translation>방식</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="83"/>
      <source>Skin</source>
      <translation>스킨</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="88"/>
      <source>Pipe</source>
      <translation>파이프</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="93"/>
      <source>Recto verso</source>
      <translation>양면</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="101"/>
      <source>Join type</source>
      <translation>결합 유형</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="109"/>
      <source>Arc</source>
      <translation>호</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="114"/>
      <location filename="../../TaskThicknessParameters.ui" line="124"/>
      <source>Intersection</source>
      <translation>교차</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.ui" line="131"/>
      <source>Make thickness inwards</source>
      <translation>안쪽으로 두께 생성</translation>
    </message>
    <message>
      <location filename="../../TaskThicknessParameters.cpp" line="269"/>
      <source>Empty thickness created!
</source>
      <translation>두께가 생성되지 않았습니다!
</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedParameters</name>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="111"/>
      <source>Remove</source>
      <translation>제거</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="404"/>
      <source>Normal sketch axis</source>
      <translation>일반 스케치 축</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="403"/>
      <source>Vertical sketch axis</source>
      <translation>수직 스케치 축</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="402"/>
      <source>Horizontal sketch axis</source>
      <translation>수평 스케치 축</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="406"/>
      <location filename="../../TaskTransformedParameters.cpp" line="442"/>
      <source>Construction line %1</source>
      <translation>보조선</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="420"/>
      <source>Base X-axis</source>
      <translation>기준 X축</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="421"/>
      <source>Base Y-axis</source>
      <translation>기준 Y축</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="422"/>
      <source>Base Z-axis</source>
      <translation>기준 Z축</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="456"/>
      <source>Base XY-plane</source>
      <translation>기준 XY 평면</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="457"/>
      <source>Base YZ-plane</source>
      <translation>기준 YZ 평면</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="458"/>
      <source>Base XZ-plane</source>
      <translation>기준 XZ 평면</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.cpp" line="430"/>
      <location filename="../../TaskTransformedParameters.cpp" line="466"/>
      <source>Select reference…</source>
      <translation>참조 선택…</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="35"/>
      <source>Transform body</source>
      <translation>바디를 이동 변환</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="48"/>
      <source>Transform tool shapes</source>
      <translation>도구 형상 변환</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="78"/>
      <source>Add Feature</source>
      <translation>피처 추가</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="88"/>
      <source>Remove Feature</source>
      <translation>피처 제거</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="122"/>
      <source>Recompute on change</source>
      <translation>변경시 재계산</translation>
    </message>
    <message>
      <location filename="../../TaskTransformedParameters.ui" line="106"/>
      <source>List can be reordered by dragging</source>
      <translation>드래그하여 목록을 재정렬할 수 있습니다.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeature</name>
    <message>
      <location filename="../../CommandBody.cpp" line="917"/>
      <source>Select Body</source>
      <translation>바디 선택</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="918"/>
      <source>Select a body from the list</source>
      <translation>목록에서 바디 선택</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_MoveFeatureInTree</name>
    <message>
      <location filename="../../CommandBody.cpp" line="1106"/>
      <source>Move Feature After…</source>
      <translation>피처를 다음으로 이동…</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1107"/>
      <source>Select a feature from the list</source>
      <translation>목록에서 피처를 선택하세요</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1194"/>
      <source>Move Tip</source>
      <translation>팁 이동</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1200"/>
      <source>Set tip to last feature?</source>
      <translation>마지막 피처를 팁으로 설정할까요?</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1195"/>
      <source>The moved feature appears after the currently set tip.</source>
      <translation>이동된 피처은 현재 설정된 팁 뒤에 나타납니다.</translation>
    </message>
  </context>
  <context>
    <name>QObject</name>
    <message>
      <location filename="../../Command.cpp" line="149"/>
      <source>Invalid selection</source>
      <translation>잘못된 선택</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="150"/>
      <source>There are no attachment modes that fit selected objects. Select something else.</source>
      <translation>선택한 객체에 적합한 부착 방법이 없습니다. 다른 것을 선택하세요.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="163"/>
      <location filename="../../Command.cpp" line="171"/>
      <location filename="../../Command.cpp" line="178"/>
      <source>Error</source>
      <translation>오류</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="812"/>
      <source>Several sub-elements selected</source>
      <translation>하부 요소들이 선택되었습니다.</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="813"/>
      <source>Select a single face as support for a sketch!</source>
      <translation>스케치의 받침으로 하나의 면을 선택하세요!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="820"/>
      <source>Select a face as support for a sketch!</source>
      <translation>스케치의 받침으로 사용할 면을 선택하세요!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="827"/>
      <source>Need a planar face as support for a sketch!</source>
      <translation>스케치의 받침으로는 평면이 필요합니다!</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="834"/>
      <source>Create a plane first or select a face to sketch on</source>
      <translation>먼저, 스케치를 그릴 평면을 생성하거나 기존의 면을 선택하세요</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="819"/>
      <source>No support face selected</source>
      <translation>선택한 받침면이 없습니다.</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="826"/>
      <source>No planar support</source>
      <translation>평평한 받침이 없음</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="833"/>
      <source>No valid planes in this document</source>
      <translation>이 문서에는 유효한 평면이 없습니다.</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="731"/>
      <location filename="../../ViewProviderDatum.cpp" line="259"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="97"/>
      <location filename="../../ViewProvider.cpp" line="137"/>
      <location filename="../../Command.cpp" line="1142"/>
      <source>A dialog is already open in the task panel</source>
      <translation>작업창에 이미 대화상자가 열려 있습니다.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="996"/>
      <source>Cannot use this command as there is no solid to subtract from.</source>
      <translation>빼기 작업을 수행할 솔리드 바디가 없으므로 이 명령을 사용할 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="999"/>
      <source>Ensure that the body contains a feature before attempting a subtractive command.</source>
      <translation>뺄셈 명령을 시도하기 전에 먼저 바디에 피처이 있는지 확인하세요</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1023"/>
      <source>Cannot use selected object. Selected object must belong to the active body</source>
      <translation>선택한 객체를사용할 수 없습니다. 선택한 객체는 활성화된 바디에 속해 있어야 합니다</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="164"/>
      <source>There is no active body. Please activate a body before inserting a datum entity.</source>
      <translation>활성 바디가 없습니다. 기준 요소를 삽입하기 전에 바디를 활성화하십시오.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="470"/>
      <source>Sub-shape binder</source>
      <translation>하위 형상 바인더</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1055"/>
      <source>No sketch to work on</source>
      <translation>작업을 수행할 스케치가 없습니다.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1056"/>
      <source>No sketch is available in the document</source>
      <translation>이 문서에는 사용할 수 있는 스케치가 없습니다.</translation>
    </message>
    <message>
      <location filename="../../SketchWorkflow.cpp" line="732"/>
      <location filename="../../ViewProviderDatum.cpp" line="260"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="98"/>
      <location filename="../../ViewProvider.cpp" line="138"/>
      <location filename="../../Command.cpp" line="1143"/>
      <source>Close this dialog?</source>
      <translation>이 대화창을 닫을까요?</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1825"/>
      <location filename="../../Command.cpp" line="1860"/>
      <source>Wrong selection</source>
      <translation>잘못 된 선택</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1826"/>
      <source>Select an edge, face, or body from a single body.</source>
      <translation>하나의 바디에서 에지, 면 또는 바디를 선택하십시오.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1833"/>
      <location filename="../../Command.cpp" line="2195"/>
      <source>Selection is not in the active body</source>
      <translation>활성화된 바디에서 선택하지 않았습니다</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1861"/>
      <source>Shape of the selected part is empty</source>
      <translation>선택한 부품의 형상이 비어있습니다</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1834"/>
      <source>Select an edge, face, or body from an active body.</source>
      <translation>활성 바디에서 에지, 면 또는 바디를 선택하십시오.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1026"/>
      <source>Consider using a shape binder or a base feature to reference external geometry in a body</source>
      <translation>바디에서 외부 기하 형상을 참조하려면 형상 바인더나 기준 피처를 사용하는 것이 좋습니다</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1847"/>
      <source>Wrong object type</source>
      <translation>잘못된 객체 유형</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="1848"/>
      <source>%1 works only on parts.</source>
      <translation>%1 파트에서만 사용 가능합니다.</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2196"/>
      <source>Please select only one feature in an active body.</source>
      <translation>활성 바디에서 피처 하나만 선택하십시오.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="73"/>
      <source>Part creation failed</source>
      <translation>파트 생성 실패</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="74"/>
      <source>Failed to create a part object.</source>
      <translation>부품 객체를 생성하는데 실패하였습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="127"/>
      <location filename="../../CommandBody.cpp" line="135"/>
      <location filename="../../CommandBody.cpp" line="151"/>
      <location filename="../../CommandBody.cpp" line="217"/>
      <source>Bad base feature</source>
      <translation>잘못된 기반 피처</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="128"/>
      <source>A body cannot be based on a Part Design feature.</source>
      <translation>바디는 파트 디자인 피처를 기준으로 할 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="136"/>
      <source>%1 already belongs to a body and cannot be used as a base feature for another body.</source>
      <translation>%1은(는) 이미 바디에 속해 있으므로 다른 바디의 기준 피처로 사용할 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="152"/>
      <source>Base feature (%1) belongs to other part.</source>
      <translation>기반 피처(%1) 은 다른 부품에 속합니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="179"/>
      <source>The selected shape consists of multiple solids.
This may lead to unexpected results.</source>
      <translation>선택한 형상은 여러 고체들로 구성됩니다.
이로 인해 예기치 않은 결과가 발생할 수 있습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="185"/>
      <source>The selected shape consists of multiple shells.
This may lead to unexpected results.</source>
      <translation>선택한 형상은 여러 껍질들로 구성됩니다.
이로 인해 예기치 않은 결과가 발생할 수 있습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="191"/>
      <source>The selected shape consists of only a shell.
This may lead to unexpected results.</source>
      <translation>선택한 형상은 껍질로만 구성됩니다.
이로 인해 예기치 않은 결과가 발생할 수 있습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="197"/>
      <source>The selected shape consists of multiple solids or shells.
This may lead to unexpected results.</source>
      <translation>선택한 형상은 여러 고체 또는 껍질로 구성됩니다.
이로 인해 예기치 않은 결과가 발생할 수 있습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="206"/>
      <source>Base feature</source>
      <translation>기반 피처</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="218"/>
      <source>Body may be based on no more than one feature.</source>
      <translation>바디는 둘 이상의 피처를 기준으로 할 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="233"/>
      <source>Body</source>
      <translation>바디</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="428"/>
      <source>Nothing to migrate</source>
      <translation>이전할 것이 없습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="697"/>
      <source>Select exactly one Part Design feature or a body.</source>
      <translation>피처 또는 바디를 정확히 하나만 선택하세요.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="705"/>
      <source>Could not determine a body for the selected feature '%s'.</source>
      <translation>선택한 피처 '%s'의 바디를 확인할 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="886"/>
      <source>Only features of a single source body can be moved</source>
      <translation>단일 원본 바디의 피처만 이동할 수 있습니다</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="621"/>
      <source>Sketch plane cannot be migrated</source>
      <translation>스케치 평면은 이전할 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="429"/>
      <source>No Part Design features without body found Nothing to migrate.</source>
      <translation>바디가 없는 파트 디자인 피처를 찾지 못했습니다. 마이그레이션할 항목이 없습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="622"/>
      <source>Please edit '%1' and redefine it to use a Base or Datum plane as the sketch plane.</source>
      <translation>기본 또는 기준 평면을 스케치 평면으로 사용하도록 '%1'을(를) 편집하고 재정의하십시오.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="696"/>
      <location filename="../../CommandBody.cpp" line="704"/>
      <location filename="../../CommandBody.cpp" line="718"/>
      <location filename="../../CommandBody.cpp" line="1072"/>
      <location filename="../../CommandBody.cpp" line="1082"/>
      <source>Selection error</source>
      <translation>선택 오류</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="719"/>
      <source>Only a solid feature can be the tip of a body.</source>
      <translation>고체 피처만 바디의 팁이 될 수 있습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="855"/>
      <location filename="../../CommandBody.cpp" line="885"/>
      <location filename="../../CommandBody.cpp" line="903"/>
      <source>Features cannot be moved</source>
      <translation>도형특징을 이동할 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="856"/>
      <source>Some of the selected features have dependencies in the source body</source>
      <translation>선택한 피처 중 일부가 원래의 바디에 종속되어 있습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="904"/>
      <source>There are no other bodies to move to</source>
      <translation>이동할 다른 바디가 없습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1073"/>
      <source>Impossible to move the base feature of a body.</source>
      <translation>바디의 기반 피처은 이동이 불가능합니다.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1083"/>
      <source>Select one or more features from the same body.</source>
      <translation>같은 바디에서 하나 이상의 피처를 선택하십시오.</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1098"/>
      <source>Beginning of the body</source>
      <translation>바디의 시작</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1179"/>
      <source>Dependency violation</source>
      <translation>종속성 위반</translation>
    </message>
    <message>
      <location filename="../../CommandBody.cpp" line="1180"/>
      <source>Early feature must not depend on later feature.

</source>
      <translation>앞의 피처은 뒤의 피처에 의존하지 않아야 합니다.

</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="309"/>
      <source>No previous feature found</source>
      <translation>이전 피처 찾을 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="310"/>
      <source>It is not possible to create a subtractive feature without a base feature available</source>
      <translation>사용 가능한 기준 피처 없이 제거 피처를 만들 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="208"/>
      <location filename="../../TaskTransformedParameters.cpp" line="439"/>
      <source>Vertical sketch axis</source>
      <translation>수직 스케치 축</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="209"/>
      <location filename="../../TaskTransformedParameters.cpp" line="440"/>
      <source>Horizontal sketch axis</source>
      <translation>수평 스케치 축</translation>
    </message>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="211"/>
      <source>Construction line %1</source>
      <translation>보조선</translation>
    </message>
    <message>
      <location filename="../../TaskSketchBasedParameters.cpp" line="96"/>
      <source>Face</source>
      <translation>면</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="208"/>
      <source>Active Body Required</source>
      <translation>활성 바디 필요</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="150"/>
      <source>To use Part Design, an active body is required in the document. Activate a body (double-click) or create a new one.

For legacy documents with Part Design objects lacking a body, use the migrate function in Part Design to place them into a body.</source>
      <translation>파트 디자인을 사용하려면 문서에 활성 바디가 있어야 합니다. 바디를 활성화(더블클릭)하거나 새로 만드십시오.

바디가 없는 파트 디자인 객체가 있는 레거시 문서는 파트 디자인의 마이그레이션 기능을 사용하여 바디에 배치하십시오.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="209"/>
      <source>To create a new Part Design object, an active body is required in the document. Activate an existing body (double-click) or create a new one.</source>
      <translation>새 파트 디자인 객체를 만들려면 문서에 활성 바디가 있어야 합니다. 기존 바디를 활성화(더블클릭)하거나 새로 만드십시오.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="275"/>
      <source>Feature is not in a body</source>
      <translation>바디 안에 피처이 없습니다.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="276"/>
      <source>In order to use this feature it needs to belong to a body object in the document.</source>
      <translation>이 피처를 사용하려면 문서의 바디 객체에 속해 있어야 합니다.</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="322"/>
      <source>Feature is not in a part</source>
      <translation>부품 안에 피처이 없습니다</translation>
    </message>
    <message>
      <location filename="../../Utils.cpp" line="323"/>
      <source>In order to use this feature it needs to belong to a part object in the document.</source>
      <translation>이 피처를 사용하려면 문서의 파트 객체에 속해 있어야 합니다.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderTransformed.cpp" line="65"/>
      <location filename="../../ViewProviderShapeBinder.cpp" line="227"/>
      <location filename="../../ViewProviderDressUp.cpp" line="64"/>
      <location filename="../../ViewProvider.cpp" line="94"/>
      <source>Edit %1</source>
      <translation>수정</translation>
    </message>
    <message>
      <location filename="../../ViewProvider.cpp" line="107"/>
      <source>Set Face Colors</source>
      <translation>면 색상 설정</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="114"/>
      <location filename="../../ViewProviderDatum.cpp" line="214"/>
      <source>Plane</source>
      <translation>평면</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="119"/>
      <location filename="../../ViewProviderDatum.cpp" line="209"/>
      <source>Line</source>
      <translation>선</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="124"/>
      <location filename="../../ViewProviderDatum.cpp" line="219"/>
      <source>Point</source>
      <translation>점</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="129"/>
      <source>Coordinate System</source>
      <translation>좌표계</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="236"/>
      <source>Edit Datum</source>
      <translation>작업기준 수정</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="93"/>
      <source>Feature error</source>
      <translation>도형특징 오류</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDressUp.cpp" line="94"/>
      <source>%1 misses a base feature.
This feature is broken and cannot be edited.</source>
      <translation>%1에 기준 피처가 없습니다.
이 피처는 손상되어 편집할 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="222"/>
      <source>Edit Shape Binder</source>
      <translation>형상 바인더 편집</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="352"/>
      <source>Synchronize</source>
      <translation>동기화</translation>
    </message>
    <message>
      <location filename="../../ViewProviderShapeBinder.cpp" line="354"/>
      <source>Select Bound Object</source>
      <translation>바인딩할 객체 선택</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="156"/>
      <source>The document "%1" you are editing was designed with an old version of Part Design workbench.</source>
      <translation>현재 편집 중인 문서 "%1"은(는) 오래된 버전의 파트 디자인 작업대로 만들어졌습니다.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="163"/>
      <source>Migrate in order to use modern Part Design features?</source>
      <translation>현대적인 파트 디자인 피처를 사용하려면 마이그레이션하시겠습니까?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="168"/>
      <source>The document "%1" seems to be either in the middle of the migration process from legacy Part Design or have a slightly broken structure.</source>
      <translation>문서 "%1"은(는) 레거시 파트 디자인에서 마이그레이션하는 중이거나 구조가 약간 손상된 것 같습니다.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="175"/>
      <source>Make the migration automatically?</source>
      <translation>마이그레이션을 자동으로 수행하시겠습니까?</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="178"/>
      <source>Note: If you choose to migrate you won't be able to edit the file with an older FreeCAD version.
If you refuse to migrate you won't be able to use new PartDesign features like Bodies and Parts. As a result you also won't be able to use your parts in the assembly workbench.
Although you will be able to migrate any moment later with 'Part Design -&gt; Migrate'.</source>
      <translation>참고: 마이그레이션을 선택하면 이전 FreeCAD 버전으로는 이 파일을 편집할 수 없습니다.
마이그레이션을 거부하면 바디와 파트 같은 새로운 파트 디자인 피처를 사용할 수 없습니다. 따라서 어셈블리 작업대에서도 부품을 사용할 수 없습니다.
나중에 언제든지 '파트 디자인 -&gt; 마이그레이션' 명령으로 마이그레이션할 수 있습니다.</translation>
    </message>
    <message>
      <location filename="../../WorkflowManager.cpp" line="191"/>
      <source>Migrate Manually</source>
      <translation>수동으로 마이그레이션</translation>
    </message>
    <message>
      <location filename="../../ViewProviderBoolean.cpp" line="70"/>
      <source>Edit Boolean</source>
      <translation>부울 편집</translation>
    </message>
    <message>
      <location filename="../../ViewProviderChamfer.cpp" line="42"/>
      <source>Edit Chamfer</source>
      <translation>모따기 수정</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDraft.cpp" line="43"/>
      <source>Edit Draft</source>
      <translation>구배 수정</translation>
    </message>
    <message>
      <location filename="../../ViewProviderFillet.cpp" line="42"/>
      <source>Edit Fillet</source>
      <translation>모깎기 수정</translation>
    </message>
    <message>
      <location filename="../../ViewProviderGroove.cpp" line="45"/>
      <source>Edit Groove</source>
      <translation>회전 홈파기 수정</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHelix.cpp" line="50"/>
      <source>Edit Helix</source>
      <translation>나선 수정</translation>
    </message>
    <message>
      <location filename="../../ViewProviderHole.cpp" line="129"/>
      <source>Edit Hole</source>
      <translation>구멍 수정</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLinearPattern.cpp" line="40"/>
      <source>Edit Linear Pattern</source>
      <translation>선형 패턴 편집</translation>
    </message>
    <message>
      <location filename="../../ViewProviderLoft.cpp" line="67"/>
      <source>Edit Loft</source>
      <translation>로프트 편집</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMirrored.cpp" line="40"/>
      <source>Edit Mirror</source>
      <translation>대칭 복사 편집</translation>
    </message>
    <message>
      <location filename="../../ViewProviderMultiTransform.cpp" line="49"/>
      <source>Edit Multi-Transform</source>
      <translation>다중 변환 편집</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPad.cpp" line="45"/>
      <source>Edit Pad</source>
      <translation>패드 수정</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPipe.cpp" line="77"/>
      <source>Edit Pipe</source>
      <translation>파이프 수정</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPocket.cpp" line="47"/>
      <source>Edit Pocket</source>
      <translation>포켓 수정</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPolarPattern.cpp" line="40"/>
      <source>Edit Polar Pattern</source>
      <translation>원형 패턴 편집</translation>
    </message>
    <message>
      <location filename="../../ViewProviderPrimitive.cpp" line="52"/>
      <source>Edit Primitive</source>
      <translation>프리미티브 편집</translation>
    </message>
    <message>
      <location filename="../../ViewProviderRevolution.cpp" line="45"/>
      <source>Edit Revolution</source>
      <translation>회전 편집</translation>
    </message>
    <message>
      <location filename="../../ViewProviderScaled.cpp" line="40"/>
      <source>Edit Scale</source>
      <translation>크기 조정 편집</translation>
    </message>
    <message>
      <location filename="../../ViewProviderThickness.cpp" line="42"/>
      <source>Edit Thickness</source>
      <translation>두께 수정</translation>
    </message>
  </context>
  <context>
    <name>SprocketParameter</name>
    <message>
      <location filename="../../../SprocketFeature.ui" line="14"/>
      <source>Sprocket Parameters</source>
      <translation>스프로킷 매개변수</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="26"/>
      <source>Number of teeth</source>
      <translation>잇수</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="52"/>
      <source>Sprocket reference</source>
      <translation>스프로킷 기준</translation>
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
      <translation>기어변속기가 있는 자전거</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="141"/>
      <source>Bicycle without derailleur</source>
      <translation>기어변속기가 없는 자전거</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="219"/>
      <source>Chain pitch</source>
      <translation>체인 피치</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="266"/>
      <source>Chain roller diameter</source>
      <translation>체인 롤러 지름</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="310"/>
      <source>Tooth width</source>
      <translation>이 폭</translation>
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
      <translation>오토바이-420</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="186"/>
      <source>Motorcycle 425</source>
      <translation>오토바이-425</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="191"/>
      <source>Motorcycle 428</source>
      <translation>오토바이-428</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="196"/>
      <source>Motorcycle 520</source>
      <translation>오토바이 520</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="201"/>
      <source>Motorcycle 525</source>
      <translation>오토바이 525</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="206"/>
      <source>Motorcycle 530</source>
      <translation>오토바이 530</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.ui" line="211"/>
      <source>Motorcycle 630</source>
      <translation>오토바이 630</translation>
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
      <location filename="../../TaskHoleParameters.ui" line="824"/>
      <source>Live update of changes to the thread
Note that the calculation can take some time</source>
      <translation>나사산의 변경사항을 실시간으로 업데이트
계산에 시간이 걸릴 수 있다는 점을 참고하세요</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1019"/>
      <source>Thread Depth</source>
      <translation>나사산 깊이</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1072"/>
      <source>Customize thread clearance</source>
      <translation>나사산의 여유간격을 사용자 정의하다</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="699"/>
      <source>Clearance</source>
      <translation>공차</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="65"/>
      <source>Head type</source>
      <translation>머리 유형</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="153"/>
      <source>Depth type</source>
      <translation>깊이 유형</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="245"/>
      <source>Head diameter</source>
      <translation>머리 지름</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="293"/>
      <source>Head depth</source>
      <translation>머리 깊이</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="668"/>
      <source>Clearance / Passthrough</source>
      <translation>클리어런스 / 관통</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="686"/>
      <source>Hole type</source>
      <translation>구멍 유형</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="828"/>
      <source>Update thread view</source>
      <translation>나사산 보기 새로고침</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1078"/>
      <source>Custom Clearance</source>
      <translation>사용자 정의 공차</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="1091"/>
      <source>Custom Thread clearance value</source>
      <translation>사용자 정의된 나사산의 여유간격 값</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="881"/>
      <source>Direction</source>
      <translation>방향</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="49"/>
      <source>Size</source>
      <translation>크기</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="712"/>
      <source>Hole clearance
Only available for holes without thread</source>
      <translation>구멍 여유간격
나사산이 없는 구멍에만 사용할 수 있습니다</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="88"/>
      <location filename="../../TaskHoleParameters.ui" line="717"/>
      <source>Standard</source>
      <translation>표준</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="673"/>
      <source>Tap drill</source>
      <translation type="unfinished">Tap drill</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="678"/>
      <source>Threaded</source>
      <translation type="unfinished">Threaded</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="722"/>
      <source>Close</source>
      <translation>닫기</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="727"/>
      <source>Wide</source>
      <translation>넓다</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="805"/>
      <source>Whether the hole gets a modelled thread</source>
      <translation type="unfinished">Whether the hole gets a modelled thread</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="808"/>
      <source>Model Thread</source>
      <translation type="unfinished">Model Thread</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="861"/>
      <source>Class</source>
      <translation>클래스</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="848"/>
      <source>Tolerance class for threaded holes according to hole profile</source>
      <translation>홀 프로파일에 따른 나사 구멍용 허용 클래스</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="552"/>
      <source>Diameter</source>
      <translation>지름</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="574"/>
      <source>Hole diameter</source>
      <translation>구멍 지름</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="507"/>
      <source>Depth</source>
      <translation>깊이</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="20"/>
      <source>Hole Parameters</source>
      <translation>구멍 매개변수</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="95"/>
      <source>Base profile types</source>
      <translation>기본 프로파일 유형</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="126"/>
      <source>Circles and arcs</source>
      <translation>원과 호</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="131"/>
      <source>Points, circles and arcs</source>
      <translation>점, 원, 호</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="136"/>
      <source>Points</source>
      <translation>점</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="170"/>
      <location filename="../../TaskHoleParameters.ui" line="989"/>
      <source>Dimension</source>
      <translation>치수</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="175"/>
      <source>Through all</source>
      <translation>관통</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="197"/>
      <source>Custom head values</source>
      <translation>사용자 지정 머리 값</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="367"/>
      <source>Drill angle</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>드릴 각도</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="403"/>
      <source>Include in depth</source>
      <extracomment>Translate it as short as possible</extracomment>
      <translation>깊이에 포함</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="610"/>
      <source>Switch direction</source>
      <translation>방향 전환</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="773"/>
      <source>Thread</source>
      <translation>나사</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="905"/>
      <source>&amp;Right hand</source>
      <translation>&amp;오른나사</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="921"/>
      <source>&amp;Left hand</source>
      <translation>&amp;왼나사</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="970"/>
      <source>Thread Depth Type</source>
      <translation>나사산 깊이 유형</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="984"/>
      <source>Hole depth</source>
      <translation>구멍 깊이</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="994"/>
      <source>Tapped (DIN76)</source>
      <translation>탭 가공(DIN76)</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="118"/>
      <source>Cut type for screw heads</source>
      <translation>나사 머리의 절단 형태</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="191"/>
      <source>Check to override the values predefined by the 'Type'</source>
      <translation>‘타입’에서 미리 정의된 값들을 재정의하는지 확인하세요.</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="306"/>
      <source>For countersinks this is the depth of
the screw's top below the surface</source>
      <translation>카운터싱크의 경우, 이것은 나사의 윗면이 표면 아래의 깊이입니다.</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="462"/>
      <source>Countersink angle</source>
      <translation>접시형 구멍 각도</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="399"/>
      <source>The size of the drill point will be taken into
account for the depth of blind holes</source>
      <translation>드릴 끝의 크기는 막힌구멍의 깊이에 영향을 받습니다</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="626"/>
      <source>Tapered</source>
      <translation>테이퍼</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="639"/>
      <source>Taper angle for the hole
90 degree: straight hole
under 90: smaller hole radius at the bottom
over 90: larger hole radius at the bottom</source>
      <translation>구멍의 테이퍼 각도
90도: 직선 구멍
90도 미만: 바닥의 구멍 반지름이 더 작음
90도 초과: 바닥의 구멍 반지름이 더 큼</translation>
    </message>
    <message>
      <location filename="../../TaskHoleParameters.ui" line="607"/>
      <source>Reverses the hole direction</source>
      <translation>구멍방향 반전</translation>
    </message>
  </context>
  <context>
    <name>TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.ui" line="25"/>
      <source>No message</source>
      <translation>메세지 없음</translation>
    </message>
  </context>
  <context>
    <name>Workbench</name>
    <message>
      <location filename="../../Workbench.cpp" line="43"/>
      <source>&amp;Sketch</source>
      <translation>스케치(&amp;S)</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="45"/>
      <source>&amp;Part Design</source>
      <translation>파트 디자인(&amp;P)</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="46"/>
      <source>Datums</source>
      <translation>기준</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="47"/>
      <source>Additive Features</source>
      <translation>덧셈 피처</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="48"/>
      <source>Subtractive Features</source>
      <translation>뺄셈 피처</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="49"/>
      <source>Dress-Up Features</source>
      <translation>꾸밈 피처</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="50"/>
      <source>Transformation Features</source>
      <translation>변환 피처</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="51"/>
      <source>Sprocket…</source>
      <translation>스프로킷…</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="52"/>
      <source>Involute Gear</source>
      <translation>인벌류트 기어</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="54"/>
      <source>Shaft Design Wizard</source>
      <translation>축 설계 마법사</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="62"/>
      <source>Measure</source>
      <translation>측정하기</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="63"/>
      <source>Refresh</source>
      <translation>새로 고침</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="64"/>
      <source>Toggle 3D</source>
      <translation>3D 전환</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="65"/>
      <source>Part Design Helper</source>
      <translation>부품설계 도우미</translation>
    </message>
    <message>
      <location filename="../../Workbench.cpp" line="66"/>
      <source>Part Design Modeling</source>
      <translation>파트 디자인 모델링</translation>
    </message>
  </context>
  <context>
    <name>WizardShaftTable</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="48"/>
      <source>Length [mm]</source>
      <translation>길이 [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="49"/>
      <source>Diameter [mm]</source>
      <translation>지름 [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="50"/>
      <source>Inner diameter [mm]</source>
      <translation>내경 [mm]</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="51"/>
      <source>Constraint type</source>
      <translation>구속 유형</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="52"/>
      <source>Start edge type</source>
      <translation>시작 에지 유형</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="53"/>
      <source>Start edge size</source>
      <translation>시작 에지 크기</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="54"/>
      <source>End edge type</source>
      <translation>끝 에지 유형</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="55"/>
      <source>End edge size</source>
      <translation>끝 에지 크기</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="69"/>
      <source>Shaft Wizard</source>
      <translation>샤프트 마법사</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="77"/>
      <source>Section 1</source>
      <translation>단면 1</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="78"/>
      <source>Section 2</source>
      <translation>단면 2</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="82"/>
      <source>Add column</source>
      <translation>열 추가</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="130"/>
      <source>Section %s</source>
      <translation>단면 %s</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="159"/>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="178"/>
      <source>None</source>
      <translation>없음</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="160"/>
      <source>Fixed</source>
      <translation>고정된</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="161"/>
      <source>Force</source>
      <translation>힘</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="162"/>
      <source>Bearing</source>
      <translation>베어링</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="163"/>
      <source>Gear</source>
      <translation>톱니바퀴</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="164"/>
      <source>Pulley</source>
      <translation>풀리</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="181"/>
      <source>Chamfer</source>
      <translation>모따기</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaftTable.py" line="182"/>
      <source>Fillet</source>
      <translation>모깎기</translation>
    </message>
  </context>
  <context>
    <name>TaskWizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="60"/>
      <source>All</source>
      <translation>전체</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="120"/>
      <source>Missing Module</source>
      <translation>모듈 누락</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="126"/>
      <source>The Plot add-on is not installed. Install it to enable this feature.</source>
      <translation>Plot 애드온이 설치되어 있지 않습니다. 이 기능을 사용하려면 설치하십시오.</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaftCallBack</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="253"/>
      <source>Shaft design wizard...</source>
      <translation>축 설계 마법사...</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="256"/>
      <source>Start the shaft design wizard</source>
      <translation>축 설계 마법사 시작하기</translation>
    </message>
  </context>
  <context>
    <name>Exception</name>
    <message>
      <location filename="../../../App/Body.cpp" line="405"/>
      <source>Linked object is not a PartDesign feature</source>
      <translation>연결된 객체가 부품설계 작업대에서 생성된 피처이 아닙니다</translation>
    </message>
    <message>
      <location filename="../../../App/Body.cpp" line="414"/>
      <source>Tip shape is empty</source>
      <translation>팁의 형상이 비어있습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="68"/>
      <source>BaseFeature link is not set</source>
      <translation>기반 피처에 대한 연결이 설정되지 않았습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="74"/>
      <source>BaseFeature must be a Part::Feature</source>
      <translation>기반 피처은 부품 작업대에서 생성된 피처 이어야 합니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBase.cpp" line="84"/>
      <source>BaseFeature has an empty shape</source>
      <translation>기반 특징에 빈 형상이 있습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="77"/>
      <source>Cannot do boolean cut without BaseFeature</source>
      <translation>기반 피처이 없이는 불리언 자르기 연산을 할 수 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="94"/>
      <source>Cannot do boolean with anything but Part::Feature and its derivatives</source>
      <translation>Part::Feature 및 그 파생형 이외에는 부울 연산을 할 수 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="106"/>
      <source>Cannot do boolean operation with invalid base shape</source>
      <translation>유효하지 않은 기본 형상으로는 부울 연산을 실행할 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="333"/>
      <location filename="../../../App/FeatureLoft.cpp" line="377"/>
      <location filename="../../../App/FeatureDraft.cpp" line="335"/>
      <location filename="../../../App/FeatureFillet.cpp" line="142"/>
      <location filename="../../../App/FeatureRevolved.cpp" line="217"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="775"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="791"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="804"/>
      <location filename="../../../App/FeaturePipe.cpp" line="482"/>
      <location filename="../../../App/FeaturePipe.cpp" line="531"/>
      <location filename="../../../App/FeatureBoolean.cpp" line="161"/>
      <location filename="../../../App/FeatureChamfer.cpp" line="196"/>
      <location filename="../../../App/FeatureHole.cpp" line="2101"/>
      <source>Result has multiple solids: enable 'Allow Compound' in the active body.</source>
      <translation>결과에 여러 솔리드가 있습니다. 활성 바디에서 '복합체 허용'을 활성화하십시오.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="116"/>
      <source>Tool shape is null</source>
      <translation>도구 형상이 비어 있습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="143"/>
      <source>Unsupported boolean operation</source>
      <translation>지원되지 않는 부울 연산</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="353"/>
      <source>Cannot create a pad with a total length of zero.</source>
      <translation>전체 길이가 0인 패드은 생성할 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="358"/>
      <source>Cannot create a pocket with a total length of zero.</source>
      <translation>전체 길이가 0인 포켓는 생성할 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="706"/>
      <source>No extrusion geometry was generated.</source>
      <translation>돌출 기하 형상이 생성되지 않았습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="730"/>
      <source>Resulting fused extrusion is null.</source>
      <translation>생성된 합집합 돌출 형상이 비어 있습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="370"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="766"/>
      <location filename="../../../App/FeaturePipe.cpp" line="523"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="141"/>
      <source>Resulting shape is not a solid</source>
      <translation>결과 형상이 고체가 아닙니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="176"/>
      <source>Failed to create chamfer</source>
      <translation>모따기 실패</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureDraft.cpp" line="330"/>
      <location filename="../../../App/FeatureFillet.cpp" line="122"/>
      <source>Resulting shape is null</source>
      <translation>결과 형상이 비어 있습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="144"/>
      <source>No edges specified</source>
      <translation>에지가 지정되지 않았습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="211"/>
      <source>Chamfer failed: OCC kernel error in chamfer computation</source>
      <translation type="unfinished">Chamfer failed: OCC kernel error in chamfer computation</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="302"/>
      <source>Size must be greater than zero</source>
      <translation>크기는 0보다 커야 합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="313"/>
      <source>Size2 must be greater than zero</source>
      <translation>크기2는 0보다 커야 합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureChamfer.cpp" line="320"/>
      <source>Angle must be greater than 0 and less than 180</source>
      <translation>각도는 0보다 크고 180보다 작아야 합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="97"/>
      <source>Fillet not possible on selected shapes</source>
      <translation>선택한 형상에 모따기를 할 수 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="105"/>
      <source>Fillet radius must be greater than zero</source>
      <translation>모깎기 반지름은 0보다 커야 합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureFillet.cpp" line="159"/>
      <source>Fillet operation failed. The selected edges may contain geometry that cannot be filleted together. Try filleting edges individually or with a smaller radius.</source>
      <translation>모깎기 작업에 실패했습니다. 선택한 에지에 함께 모깎기할 수 없는 기하 형상이 포함되어 있을 수 있습니다. 에지를 개별적으로 모깎기하거나 더 작은 반지름으로 시도하십시오.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1739"/>
      <source>The requested feature cannot be created. The reason may be that:
  - the active Body does not contain a base shape, so there is no
  material to be removed;
  - the selected sketch does not belong to the active Body.</source>
      <translation>요청한 피처를 만들 수 없습니다. 가능한 이유는 다음과 같습니다.
  - 활성 바디에 기준 형상이 없어 제거할 재질이 없습니다.
  - 선택한 스케치가 활성 바디에 속하지 않습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="402"/>
      <source>Failed to obtain profile shape</source>
      <translation>윤곽 형상을 얻는데 실패했습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="456"/>
      <source>Creation failed because direction is orthogonal to sketch's normal vector</source>
      <translation>방향이 스케치의 법선 벡터에 직교하므로 만들지 못했습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="132"/>
      <location filename="../../../App/FeatureExtrude.cpp" line="479"/>
      <source>Creating a face from sketch failed</source>
      <translation>스케치로부터 면생성 실패</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="152"/>
      <source>Revolve axis intersects the sketch</source>
      <translation>회전축이 스케치와 교차합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="202"/>
      <source>Could not revolve the sketch!</source>
      <translation>스케치를 회전시킬 수 없음!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="69"/>
      <source>Could not create face from sketch.
Intersecting sketch entities in a sketch are not allowed.</source>
      <translation>스케치로부터 면을 생성할 수 없습니다.
스케치에서 교차하는 개체들은 허용되지 않습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="237"/>
      <source>Error: Pitch too small!</source>
      <translation>오류: 너무 작은 피치!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="242"/>
      <location filename="../../../App/FeatureHelix.cpp" line="265"/>
      <source>Error: height too small!</source>
      <translation>오류: 너무 작은 높이</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="251"/>
      <source>Error: pitch too small!</source>
      <translation>오류: 너무 작은 피치!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="256"/>
      <location filename="../../../App/FeatureHelix.cpp" line="270"/>
      <location filename="../../../App/FeatureHelix.cpp" line="279"/>
      <source>Error: turns too small!</source>
      <translation>오류: 너무 작은 회전수</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="285"/>
      <source>Error: either height or growth must not be zero!</source>
      <translation>오류: 높이 또는 증가량 중 하나는 0이면 안 됩니다!</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="303"/>
      <source>Error: unsupported mode</source>
      <translation>오류: 지원되지 않는 모드</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="317"/>
      <source>Error: No valid sketch or face</source>
      <translation>오류: 스케치 혹은 면이 유효하지 않습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="330"/>
      <source>Error: Face must be planar</source>
      <translation>오류: 면은 평면이어야 합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="445"/>
      <location filename="../../../App/FeatureHelix.cpp" line="486"/>
      <location filename="../../../App/FeatureHole.cpp" line="2457"/>
      <source>Error: Result is not a solid</source>
      <translation>오류: 결과가 고체가 아닙니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="415"/>
      <source>Error: There is nothing to subtract</source>
      <translation>오류: 뺄 것이 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="421"/>
      <location filename="../../../App/FeatureHelix.cpp" line="451"/>
      <location filename="../../../App/FeatureHelix.cpp" line="492"/>
      <source>Error: Result has multiple solids</source>
      <translation>오류: 결과가 여러 고체들을 가집니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="436"/>
      <source>Error: Adding the helix failed</source>
      <translation>오류: 나선 추가 실패</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="468"/>
      <source>Error: Intersecting the helix failed</source>
      <translation>오류: 나선 교차에 실패했습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="477"/>
      <source>Error: Subtracting the helix failed</source>
      <translation>오류: 나선 빼기에 실패했습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHelix.cpp" line="508"/>
      <source>Error: Could not create face from sketch</source>
      <translation>오류: 스케치로부터 면을 생성할 수 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1233"/>
      <source>Thread type is invalid</source>
      <translation>나사산 유형이 잘못되었습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1752"/>
      <source>Hole error: Diameter too small</source>
      <translation>구멍 오류: 지름이 너무 작습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1789"/>
      <source>Hole error: Unsupported length specification</source>
      <translation>구멍 오류: 지원되지 않는 길이 사양</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1795"/>
      <source>Hole error: Invalid hole depth</source>
      <translation>구멍 오류: 무효한 구멍 깊이</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1821"/>
      <source>Hole error: Invalid taper angle</source>
      <translation>구멍 오류: 잘못된 테이퍼 각도</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1845"/>
      <source>Hole error: Hole cut diameter too small</source>
      <translation>구멍 오류: 구멍파기 지름이 너무 작습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1850"/>
      <source>Hole error: Hole cut depth must be less than hole depth</source>
      <translation>구멍 오류: 구멍파기 깊이는 구멍의 전체 깊이보다 작아야 합니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1857"/>
      <source>Hole error: Hole cut depth must be greater or equal to zero</source>
      <translation>구멍 오류: 구멍파기 깊이는 0 이상이어야 합니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1887"/>
      <source>Hole error: Invalid countersink</source>
      <translation>구멍 오류: 무효한 접시형 구멍</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1923"/>
      <source>Hole error: Invalid drill point angle</source>
      <translation>구멍 오류: 무효한 드릴 끝 각도</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1940"/>
      <source>Hole error: Invalid drill point</source>
      <translation>구멍 오류: 무효한 드릴 끝</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1977"/>
      <source>Hole error: Could not revolve sketch</source>
      <translation>구멍 오류: 스케치를 회전시킬 수 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="1984"/>
      <source>Hole error: Resulting shape is empty</source>
      <translation>구멍 오류: 결과 형상이 비어 있습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2007"/>
      <source>Hole error: Finding axis failed</source>
      <translation>구멍 오류: 축을 찾지 못했습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2073"/>
      <location filename="../../../App/FeatureHole.cpp" line="2081"/>
      <source>Boolean operation failed on profile Edge</source>
      <translation>프로파일 에지에서 부울 연산에 실패했습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2088"/>
      <source>Boolean operation produced non-solid on profile Edge</source>
      <translation>프로파일 에지에서 부울 연산 결과가 비솔리드였습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureBoolean.cpp" line="153"/>
      <source>Boolean operation failed</source>
      <translation>부울 연산 실패</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2114"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed for making a pocket up to a face.</source>
      <translation>스케치로부터 면을 생성할 수 없습니다.
스케치에 교차하는 선이나 또는 다수의 면이 있으면 면까지 포켓를 만들 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2280"/>
      <source>Thread type out of range</source>
      <translation>나사산의 유형이 범위를 벗어납니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2283"/>
      <source>Thread size out of range</source>
      <translation>나사산의 크기가 범위를 벗어납니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureHole.cpp" line="2431"/>
      <source>Error: Thread could not be built</source>
      <translation>오류: 나사산을 만들 수 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="193"/>
      <source>Loft: At least one section is needed</source>
      <translation>로프트: 최소 하나의 단면이 필요합니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="394"/>
      <source>Loft: A fatal error occurred when making the loft</source>
      <translation>로프트: 로프트 생성 중 치명적 오류 발생</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="240"/>
      <source>Loft: Creating a face from sketch failed</source>
      <translation>로프트: 스케치로부터 면 생성이 실패했습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="303"/>
      <location filename="../../../App/FeaturePipe.cpp" line="446"/>
      <source>Loft: Failed to create shell</source>
      <translation>로프트: 껍질 생성에 실패했습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="819"/>
      <source>Could not create face from sketch.
Intersecting sketch entities or multiple faces in a sketch are not allowed.</source>
      <translation>스케치로부터 면을 생성할 수 없습니다.
스케치에서 교차하는 선이나 또는 다수의 면은 허용되지 않습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="205"/>
      <source>Pipe: Could not obtain profile shape</source>
      <translation>파이프: 윤곽 형상을 얻을 수 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="212"/>
      <source>No spine linked</source>
      <translation>연결된 척추(경로) 가 없음</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="227"/>
      <source>No auxiliary spine linked.</source>
      <translation>연결된 보조 척추가 없습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="250"/>
      <source>Pipe: Only one isolated point is needed if using a sketch with isolated points for section</source>
      <translation>파이프:단면에 고립점을 갖는 스케치를 사용하려면 오직 한 개의 고립점만 필요합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="259"/>
      <source>Pipe: At least one section is needed when using a single point for profile</source>
      <translation>파이프: 하나의 점을 윤곽으로 사용하는 경우, 적어도 하나의 단면이 필요합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="277"/>
      <source>Pipe: All sections need to be Part features</source>
      <translation>파이프: 모든 단면은 Part 피처여야 합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="285"/>
      <source>Pipe: Could not obtain section shape</source>
      <translation>파이프: 단면 형상을 얻을 수 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="295"/>
      <source>Pipe: Only the profile and last section can be vertices</source>
      <translation>파이프: 오직 윤곽과 마지막 단면만이 꼭지점이 될 수 있습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="308"/>
      <source>Multisections need to have the same amount of inner wires as the base section</source>
      <translation>다중단면은 기본 단면으로서 내부에 동일한 양의 철사를 가져야 합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="341"/>
      <source>Path must not be a null shape</source>
      <translation>경로는 빈 형상이 아니어야 합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="381"/>
      <source>Pipe could not be built</source>
      <translation>파이프를 만들 수 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="438"/>
      <source>Result is not a solid</source>
      <translation>결과가 고체가 아닙니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="477"/>
      <source>Pipe: There is nothing to subtract from</source>
      <translation>파이프: 제거해야 할 대상이 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="545"/>
      <source>A fatal error occurred when making the pipe</source>
      <translation>파이프를 생성하는데 치명적 오류가 발생하였습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="674"/>
      <source>Invalid element in spine.</source>
      <translation>척추의 요소가 유효하지 않습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="679"/>
      <source>Element in spine is neither an edge nor a wire.</source>
      <translation>척추에 있는 요소는 에지도 아니고 철사도 아닙니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="700"/>
      <source>Spine is not connected.</source>
      <translation>척추가 이어지지 않았습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="706"/>
      <source>Spine is neither an edge nor a wire.</source>
      <translation>철사는 에지도 아니고 철사도 아닙니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePipe.cpp" line="711"/>
      <source>Invalid spine.</source>
      <translation>유효하지 않은 척추</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="103"/>
      <source>Cannot subtract primitive feature without base feature</source>
      <translation>기준 피처 없이 프리미티브 제거 피처를 만들 수 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="355"/>
      <location filename="../../../App/FeaturePipe.cpp" line="507"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="125"/>
      <source>Unknown operation type</source>
      <translation>알 수 없는 연산 유형</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureLoft.cpp" line="363"/>
      <location filename="../../../App/FeaturePipe.cpp" line="515"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="133"/>
      <source>Failed to perform boolean operation</source>
      <translation>부울 연산 실패</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="217"/>
      <source>Length of box too small</source>
      <translation>상자 길이가 너무 작습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="222"/>
      <source>Width of box too small</source>
      <translation>상자 폭이 너무 작습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="227"/>
      <source>Height of box too small</source>
      <translation>상자 높이가 너무 작습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="275"/>
      <source>Radius of cylinder too small</source>
      <translation>원통의 반지름이 너무 작습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="280"/>
      <source>Height of cylinder too small</source>
      <translation>원통의 높이가 너무 작습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="285"/>
      <source>Rotation angle of cylinder too small</source>
      <translation>원통의 회전 각도가 너무 작습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="342"/>
      <source>Radius of sphere too small</source>
      <translation>구의 반지름이 너무 작습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="394"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="399"/>
      <source>Radius of cone cannot be negative</source>
      <translation>원뿔의 반지름은 음수가 될 수 없습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="404"/>
      <source>Height of cone too small</source>
      <translation>원뿔의 높이가 너무 작습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="484"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="489"/>
      <source>Radius of ellipsoid too small</source>
      <translation>타원체의 반지름이 너무 작습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="583"/>
      <location filename="../../../App/FeaturePrimitive.cpp" line="588"/>
      <source>Radius of torus too small</source>
      <translation>원환체의 반지름이 너무 작습니다.</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="673"/>
      <source>Polygon of prism is invalid, must have 3 or more sides</source>
      <translation>각기둥의 다각형이 잘못되었습니다. 변이 3개 이상이어야 합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="678"/>
      <source>Circumradius of the polygon, of the prism, is too small</source>
      <translation>다각형의 원반지름, 각기둥의 원반지름이 너무 작습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="683"/>
      <source>Height of prism is too small</source>
      <translation>각기둥의 높이가 너무 작습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="770"/>
      <source>delta x of wedge too small</source>
      <translation>쐐기의 x 변화량이 너무 작습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="776"/>
      <source>delta y of wedge too small</source>
      <translation>쐐기의 y 변화량이 너무 작습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="782"/>
      <source>delta z of wedge too small</source>
      <translation>쐐기의 z 변화량이 너무 작습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="788"/>
      <source>delta z2 of wedge is negative</source>
      <translation>쐐기의 z2 변화량이 음수입니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeaturePrimitive.cpp" line="794"/>
      <source>delta x2 of wedge is negative</source>
      <translation>쐐기의 x2 변화량이 음수입니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="96"/>
      <source>Angle of revolution too large</source>
      <translation>회전 각도가 너무 큽니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="103"/>
      <source>Angle of revolution too small</source>
      <translation>회전 각도가 너무 작습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="110"/>
      <source>Angles of revolution nullify each other</source>
      <translation>회전 각도가 서로 상쇄됩니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureRevolved.cpp" line="126"/>
      <source>Reference axis is invalid</source>
      <translation>참조 축이 잘못되었습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureExtrude.cpp" line="758"/>
      <source>Fusion with base feature failed</source>
      <translation>기본 피처들 결합 실패</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="101"/>
      <source>Transformation feature Linked object is not a Part object</source>
      <translation>변환 피처의 연결 객체가 Part 객체가 아닙니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="109"/>
      <source>No features selected to be mirrored.</source>
      <translation type="unfinished">No features selected to be mirrored.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="112"/>
      <source>No features selected to be patterned.</source>
      <translation type="unfinished">No features selected to be patterned.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="115"/>
      <source>No features selected to be transformed.</source>
      <translation type="unfinished">No features selected to be transformed.</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="356"/>
      <source>Cannot transform invalid support shape</source>
      <translation>무효한 받침 형상을 변환할 수 없습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="407"/>
      <source>Shape of additive/subtractive feature is empty</source>
      <translation>덧셈/뺄셈 피처의 형태가 비어 있습니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureTransformed.cpp" line="398"/>
      <source>Only additive and subtractive features can be transformed</source>
      <translation>오직 덧셈/뺄셈 피처들만 변형이 가능합니다</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureThickness.cpp" line="109"/>
      <source>Invalid face reference</source>
      <translation>잘못된 면 참조</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_InvoluteGear</name>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="62"/>
      <source>Involute Gear</source>
      <translation>인벌류트 기어</translation>
    </message>
    <message>
      <location filename="../../../InvoluteGearFeature.py" line="66"/>
      <source>Creates or edits the involute gear definition</source>
      <translation>인벌류트 기어 정의를 생성하거나 편집합니다</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_Sprocket</name>
    <message>
      <location filename="../../../SprocketFeature.py" line="65"/>
      <source>Sprocket</source>
      <translation>스프로킷</translation>
    </message>
    <message>
      <location filename="../../../SprocketFeature.py" line="69"/>
      <source>Creates or edits the sprocket definition.</source>
      <translation>스프로킷 정의를 생성하거나 편집합니다.</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPreviewParameters</name>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="20"/>
      <source>Show final result</source>
      <translation>최종 결과 보이기</translation>
    </message>
    <message>
      <location filename="../../TaskPreviewParameters.ui" line="27"/>
      <source>Show preview overlay</source>
      <translation>미리 보기 오버레이 표시</translation>
    </message>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="50"/>
      <source>Preview</source>
      <translation>미리 보기</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_WizardShaft</name>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="225"/>
      <source>Shaft Design Wizard</source>
      <translation>축 설계 마법사</translation>
    </message>
    <message>
      <location filename="../../../WizardShaft/WizardShaft.py" line="228"/>
      <source>Starts the shaft design wizard</source>
      <translation>축 설계 마법사를 시작합니다</translation>
    </message>
  </context>
  <context>
    <name>PartDesign::FeatureAddSub</name>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="87"/>
      <source>Failure while computing removed volume preview: %1</source>
      <translation>제거 볼륨 미리 보기를 계산하는 동안 실패함: %1</translation>
    </message>
    <message>
      <location filename="../../../App/FeatureAddSub.cpp" line="125"/>
      <source>Resulting shape is empty. That may indicate that no material will be removed or a problem with the model.</source>
      <translation>결과 형상이 비어 있습니다. 제거될 재질이 없거나 모델에 문제가 있을 수 있습니다.</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompDatums</name>
    <message>
      <location filename="../../Command.cpp" line="2648"/>
      <source>Create Datum</source>
      <translation>작업기준 생성</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2649"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>작업기준 객체 또는 로컬 좌표계 생성</translation>
    </message>
  </context>
  <context>
    <name>CmdPartDesignCompSketches</name>
    <message>
      <location filename="../../Command.cpp" line="2683"/>
      <source>Create Datum</source>
      <translation>작업기준 생성</translation>
    </message>
    <message>
      <location filename="../../Command.cpp" line="2684"/>
      <source>Creates a datum object or local coordinate system</source>
      <translation>작업기준 객체 또는 로컬 좌표계 생성</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveAdditive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="217"/>
      <source>Creates an additive box by its width, height, and length</source>
      <translation>너비, 높이, 길이로 추가 박스를 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="226"/>
      <source>Creates an additive cylinder by its radius, height, and angle</source>
      <translation>반지름, 높이, 각도로 추가 원기둥을 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="235"/>
      <source>Creates an additive sphere by its radius and various angles</source>
      <translation>반지름과 여러 각도로 추가 구를 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="244"/>
      <source>Creates an additive cone</source>
      <translation>추가 원뿔을 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="250"/>
      <source>Creates an additive ellipsoid</source>
      <translation>추가 타원체를 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="256"/>
      <source>Creates an additive torus</source>
      <translation>추가 원환체를 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="262"/>
      <source>Creates an additive prism</source>
      <translation>바디에 더할 각기둥을 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="268"/>
      <source>Creates an additive wedge</source>
      <translation>추가 쐐기를 생성합니다</translation>
    </message>
  </context>
  <context>
    <name>PartDesign_CompPrimitiveSubtractive</name>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="402"/>
      <source>Creates a subtractive box by its width, height and length</source>
      <translation>너비, 높이, 길이로 제거 박스를 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="411"/>
      <source>Creates a subtractive cylinder by its radius, height and angle</source>
      <translation>반지름, 높이, 각도로 제거 원기둥을 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="420"/>
      <source>Creates a subtractive sphere by its radius and various angles</source>
      <translation>반지름과 여러 각도로 제거 구를 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="429"/>
      <source>Creates a subtractive cone</source>
      <translation>제거 원뿔을 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="435"/>
      <source>Creates a subtractive ellipsoid</source>
      <translation>제거 타원체를 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="441"/>
      <source>Creates a subtractive torus</source>
      <translation>제거 원환체를 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="447"/>
      <source>Creates a subtractive prism</source>
      <translation>제거 각기둥을 생성합니다</translation>
    </message>
    <message>
      <location filename="../../CommandPrimitive.cpp" line="453"/>
      <source>Creates a subtractive wedge</source>
      <translation>제거 쐐기를 생성합니다</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgPrimitiveParameters</name>
    <message>
      <location filename="../../TaskPrimitiveParameters.cpp" line="1082"/>
      <source>Attachment</source>
      <translation>부착</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgRevolutionParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="802"/>
      <source>Revolution Parameters</source>
      <translation>회전 매개변수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgGrooveParameters</name>
    <message>
      <location filename="../../TaskRevolutionParameters.cpp" line="812"/>
      <source>Groove Parameters</source>
      <translation>홈 매개변수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskTransformedMessages</name>
    <message>
      <location filename="../../TaskTransformedMessages.cpp" line="39"/>
      <source>Transformed Feature Messages</source>
      <translation>변환 피처 메시지</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderBody</name>
    <message>
      <location filename="../../ViewProviderBody.cpp" line="199"/>
      <source>Active Body</source>
      <translation>바디 활성화</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderChamfer</name>
    <message>
      <location filename="../../ViewProviderChamfer.h" line="44"/>
      <source>Chamfer Parameters</source>
      <translation>모따기 매개변수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDatum</name>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="115"/>
      <source>Datum Plane Parameters</source>
      <translation>기준 평면 매개변수</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="120"/>
      <source>Datum Line Parameters</source>
      <translation>기준선 매개변수</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="125"/>
      <source>Datum Point Parameters</source>
      <translation>기준점 매개변수</translation>
    </message>
    <message>
      <location filename="../../ViewProviderDatum.cpp" line="130"/>
      <source>Local Coordinate System Parameters</source>
      <translation>로컬 좌표계 매개변수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderDraft</name>
    <message>
      <location filename="../../ViewProviderDraft.h" line="45"/>
      <source>Draft Parameters</source>
      <translation>구배 매개변수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderFillet</name>
    <message>
      <location filename="../../ViewProviderFillet.h" line="44"/>
      <source>Fillet Parameters</source>
      <translation>모깎기 매개변수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderLinearPattern</name>
    <message>
      <location filename="../../ViewProviderLinearPattern.h" line="41"/>
      <source>Linear Pattern Parameters</source>
      <translation>선형 패턴 매개변수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGuii::ViewProviderMirrored</name>
    <message>
      <location filename="../../ViewProviderMirrored.h" line="41"/>
      <source>Mirror Parameters</source>
      <translation>대칭 복사 매개변수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderMultiTransform</name>
    <message>
      <location filename="../../ViewProviderMultiTransform.h" line="41"/>
      <source>Multi-Transform Parameters</source>
      <translation>다중 변환 매개변수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderPolarPattern</name>
    <message>
      <location filename="../../ViewProviderPolarPattern.h" line="41"/>
      <source>Polar Pattern Parameters</source>
      <translation>원형 패턴 매개변수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderScaled</name>
    <message>
      <location filename="../../ViewProviderScaled.h" line="41"/>
      <source>Scale Parameters</source>
      <translation>크기 조정 매개변수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::ViewProviderThickness</name>
    <message>
      <location filename="../../ViewProviderThickness.h" line="44"/>
      <source>Thickness Parameters</source>
      <translation>두께 매개변수</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskPatternParameters</name>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="132"/>
      <source>Direction 2</source>
      <translation>방향 2</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="248"/>
      <source>Select a direction reference (edge, face, datum line)</source>
      <translation>방향 기준(에지, 면, 기준선)을 선택하십시오</translation>
    </message>
    <message>
      <location filename="../../TaskPatternParameters.cpp" line="334"/>
      <source>Invalid selection. Select an edge, planar face, or datum line.</source>
      <translation>잘못된 선택입니다. 에지, 평면 면 또는 기준선을 선택하십시오</translation>
    </message>
  </context>
  <context>
    <name>PartDesignGui::TaskDlgFeatureParameters</name>
    <message>
      <location filename="../../TaskFeatureParameters.cpp" line="229"/>
      <source>The feature could not be created with the given parameters.
The geometry may be invalid or the parameters may be incompatible.
Please adjust the parameters and try again.</source>
      <translation>주어진 매개변수로 피처를 만들 수 없습니다.
기하 형상이 잘못되었거나 매개변수가 서로 호환되지 않을 수 있습니다.
매개변수를 조정한 뒤 다시 시도하십시오.</translation>
    </message>
  </context>
</TS>
