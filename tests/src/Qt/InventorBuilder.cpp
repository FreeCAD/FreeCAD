#include <Base/Builder3D.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSeparator.h>
#include <QDebug>
#include <QTest>
#include <sstream>

Q_DECLARE_METATYPE(Base::Vector3f)
Q_DECLARE_METATYPE(Base::ColorRGB)
Q_DECLARE_METATYPE(Base::Line3f)
Q_DECLARE_METATYPE(Base::BindingElement::Binding)
Q_DECLARE_METATYPE(Base::DrawStyle)
Q_DECLARE_METATYPE(Base::DrawStyle::Style)
Q_DECLARE_METATYPE(Base::PolygonOffset)
Q_DECLARE_METATYPE(Base::PolygonOffset::Style)

class testInventorBuilder: public QObject
{
    Q_OBJECT

public:
    testInventorBuilder()
        : builder(output)
    {}
    ~testInventorBuilder() override = default;

    SoNode* loadBuffer(const std::string& buffer)
    {
        SoInput in;
        in.setBuffer((void*)buffer.c_str(), buffer.size());
        return SoDB::readAll(&in);
    }

private Q_SLOTS:
    void initTestCase()
    {
        SoDB::init();
    }
    void initTestCase_data()
    {}
    void cleanupTestCase()
    {
        SoDB::finish();
    }

    void init()
    {}

    void cleanup()
    {
        // clear the buffer
        output.str(std::string());
    }

    void test_Invalid()
    {
        SoNode* node = loadBuffer("Hello, World");
        QCOMPARE(node, nullptr);
    }

    void test_Output()
    {
        std::stringstream str;
        Base::InventorBuilder builder(str);
        QCOMPARE(str.str().c_str(), "#Inventor V2.1 ascii \n\n");
    }

    void test_MaterialBinding_data()
    {
        QTest::addColumn<Base::BindingElement::Binding>("input");
        QTest::addColumn<QString>("result");
        QTest::newRow("MaterialBinding")
            << Base::BindingElement::Binding::Overall << "MaterialBinding { value OVERALL } \n";
        QTest::newRow("MaterialBinding")
            << Base::BindingElement::Binding::PerPart << "MaterialBinding { value PER_PART } \n";
        QTest::newRow("MaterialBinding") << Base::BindingElement::Binding::PerPartIndexed
                                         << "MaterialBinding { value PER_PART_INDEXED } \n";
        QTest::newRow("MaterialBinding")
            << Base::BindingElement::Binding::PerFace << "MaterialBinding { value PER_FACE } \n";
        QTest::newRow("MaterialBinding") << Base::BindingElement::Binding::PerFaceIndexed
                                         << "MaterialBinding { value PER_FACE_INDEXED } \n";
        QTest::newRow("MaterialBinding") << Base::BindingElement::Binding::PerVertex
                                         << "MaterialBinding { value PER_VERTEX } \n";
        QTest::newRow("MaterialBinding") << Base::BindingElement::Binding::PerVertexIndexed
                                         << "MaterialBinding { value PER_VERTEX_INDEXED } \n";
        QTest::newRow("MaterialBinding")
            << Base::BindingElement::Binding::Default << "MaterialBinding { value OVERALL } \n";
        QTest::newRow("MaterialBinding")
            << Base::BindingElement::Binding::None << "MaterialBinding { value OVERALL } \n";
    }

    void test_MaterialBinding()
    {
        QFETCH(Base::BindingElement::Binding, input);
        QFETCH(QString, result);

        Base::MaterialBindingItem item;
        item.setValue(input);
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_Label_data()
    {
        auto result =
            R"(Label {
  label "FreeCAD"
}
)";
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("result");
        QTest::newRow("Label") << "FreeCAD" << result;
    }

    void test_Label()
    {
        QFETCH(QString, input);
        QFETCH(QString, result);

        Base::LabelItem item {input.toStdString()};
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_Info_data()
    {
        auto result =
            R"(Info {
  string "FreeCAD"
}
)";
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("result");
        QTest::newRow("Info") << "FreeCAD" << result;
    }

    void test_Info()
    {
        QFETCH(QString, input);
        QFETCH(QString, result);

        Base::InfoItem item {input.toStdString()};
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_Text2_data()
    {
        auto result = "Text2 { string \"FreeCAD\" }\n";
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("result");
        QTest::newRow("Text2") << "FreeCAD" << result;
    }

    void test_Text2()
    {
        QFETCH(QString, input);
        QFETCH(QString, result);

        Base::Text2Item item {input.toStdString()};
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_BaseColor_data()
    {
        auto result =
            R"(BaseColor {
  rgb 0.21 0.3 0.4
}
)";
        QTest::addColumn<Base::ColorRGB>("input");
        QTest::addColumn<QString>("result");
        QTest::newRow("BaseColor") << Base::ColorRGB {0.21F, 0.3F, 0.4F} << result;
    }

    void test_BaseColor()
    {
        QFETCH(Base::ColorRGB, input);
        QFETCH(QString, result);

        Base::BaseColorItem item {input};
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_Material_data()
    {
        auto result =
            R"(Material {
  diffuseColor 1 0 0
}
)";
        QTest::addColumn<Base::ColorRGB>("input");
        QTest::addColumn<QString>("result");
        QTest::newRow("Material") << Base::ColorRGB {1, 0, 0} << result;
    }

    void test_Material()
    {
        QFETCH(Base::ColorRGB, input);
        QFETCH(QString, result);

        Base::MaterialItem item;
        item.setDiffuseColor({input});
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_Materials_data()
    {
        auto result =
            R"(Material {
  diffuseColor [
    1 0 0
    0 1 0
    0 0 1
  ]
}
)";
        QTest::addColumn<Base::ColorRGB>("input1");
        QTest::addColumn<Base::ColorRGB>("input2");
        QTest::addColumn<Base::ColorRGB>("input3");
        QTest::addColumn<QString>("result");
        QTest::newRow("Material") << Base::ColorRGB {1, 0, 0} << Base::ColorRGB {0, 1, 0}
                                  << Base::ColorRGB {0, 0, 1} << result;
    }

    void test_Materials()
    {
        QFETCH(Base::ColorRGB, input1);
        QFETCH(Base::ColorRGB, input2);
        QFETCH(Base::ColorRGB, input3);
        QFETCH(QString, result);

        Base::MaterialItem item;
        item.setDiffuseColor({input1, input2, input3});
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);

        SoNode* node = loadBuffer(output.str());
        QVERIFY(node != nullptr);
        QVERIFY(node->getRefCount() == 0);
    }

    void test_DrawStyle_data()
    {
        auto result =
            R"(DrawStyle {
  style FILLED
  pointSize 3
  lineWidth 3
  linePattern 0xf0f0
}
)";
        QTest::addColumn<Base::DrawStyle::Style>("styleEnum");
        QTest::addColumn<ushort>("pointSize");
        QTest::addColumn<ushort>("lineWidth");
        QTest::addColumn<ushort>("linePattern");
        QTest::addColumn<QString>("result");
        QTest::newRow("DrawStyle")
            << Base::DrawStyle::Style::Filled << ushort(3) << ushort(3) << ushort(0xf0f0) << result;
    }

    void test_DrawStyle()
    {
        QFETCH(Base::DrawStyle::Style, styleEnum);
        QFETCH(ushort, pointSize);
        QFETCH(ushort, lineWidth);
        QFETCH(ushort, linePattern);
        QFETCH(QString, result);

        Base::DrawStyleItem item;
        Base::DrawStyle style;
        style.style = styleEnum;
        style.pointSize = pointSize;
        style.lineWidth = lineWidth;
        style.linePattern = linePattern;
        item.setValue(style);
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_ShapeHints_data()
    {
        auto result =
            R"(ShapeHints {
  creaseAngle 0.5
}
)";
        QTest::addColumn<float>("input");
        QTest::addColumn<QString>("result");
        QTest::newRow("ShapeHints") << 0.5F << result;
    }

    void test_ShapeHints()
    {
        QFETCH(float, input);
        QFETCH(QString, result);

        Base::ShapeHintsItem item {input};
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_PolygonOffset_data()
    {
        auto result =
            R"(PolygonOffset {
  factor 2
  units 1
  styles FILLED
  on FALSE
}
)";
        QTest::addColumn<Base::PolygonOffset::Style>("styleEnum");
        QTest::addColumn<float>("factor");
        QTest::addColumn<float>("units");
        QTest::addColumn<bool>("on");
        QTest::addColumn<QString>("result");
        QTest::newRow("PolygonOffset")
            << Base::PolygonOffset::Style::Filled << 2.0F << 1.0F << false << result;
    }

    void test_PolygonOffset()
    {
        QFETCH(Base::PolygonOffset::Style, styleEnum);
        QFETCH(float, factor);
        QFETCH(float, units);
        QFETCH(bool, on);
        QFETCH(QString, result);

        Base::PolygonOffsetItem item;
        Base::PolygonOffset offset;
        offset.factor = factor;
        offset.units = units;
        offset.style = styleEnum;
        offset.on = on;
        item.setValue(offset);
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_PointSet_data()
    {
        auto result = "PointSet { }\n";
        QTest::addColumn<QString>("result");
        QTest::newRow("PointSet") << result;
    }

    void test_PointSet()
    {
        QFETCH(QString, result);

        Base::PointSetItem item;
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_LineSet_data()
    {
        auto result = "LineSet { }\n";
        QTest::addColumn<QString>("result");
        QTest::newRow("LineSet") << result;
    }

    void test_LineSet()
    {
        QFETCH(QString, result);

        Base::LineSetItem item;
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_FaceSet_data()
    {
        QTest::addColumn<int>("num");
        QTest::newRow("FaceSet") << 2;
    }

    void test_FaceSet()
    {
        QFETCH(int, num);

        Base::FaceSetItem item {{num}};
        builder.addNode(item);

        SoNode* node = loadBuffer(output.str());
        QVERIFY(node != nullptr);
    }

    void test_IndexedLineSet_data()
    {
        QTest::addColumn<int>("c1");
        QTest::addColumn<int>("c2");
        QTest::addColumn<int>("c3");
        QTest::newRow("IndexedLineSet") << 0 << 1 << 2;
    }

    void test_IndexedLineSet()
    {
        QFETCH(int, c1);
        QFETCH(int, c2);
        QFETCH(int, c3);

        Base::IndexedLineSetItem item {{c1, c2, c3}};
        builder.addNode(item);

        SoNode* node = loadBuffer(output.str());
        QVERIFY(node != nullptr);
    }

    void test_IndexedFaceSet_data()
    {
        QTest::addColumn<int>("c1");
        QTest::addColumn<int>("c2");
        QTest::addColumn<int>("c3");
        QTest::addColumn<int>("c4");
        QTest::newRow("IndexedFaceSet") << 0 << 1 << 2 << -1;
    }

    void test_IndexedFaceSet()
    {
        QFETCH(int, c1);
        QFETCH(int, c2);
        QFETCH(int, c3);
        QFETCH(int, c4);

        Base::IndexedFaceSetItem item {{c1, c2, c3, c4}};
        builder.addNode(item);

        SoNode* node = loadBuffer(output.str());
        QVERIFY(node != nullptr);
    }

    void test_Transform()
    {
        Base::Placement plm;
        Base::TransformItem item {plm};
        builder.addNode(item);

        SoNode* node = loadBuffer(output.str());
        QVERIFY(node != nullptr);
    }

    void test_Normal_data()
    {
        auto result =
            R"(Normal {
  vector 1 0 0.5
}
)";
        QTest::addColumn<Base::Vector3f>("input");
        QTest::addColumn<QString>("result");
        QTest::newRow("Normal") << Base::Vector3f {1, 0, 0.5} << result;
    }

    void test_Normal()
    {
        QFETCH(Base::Vector3f, input);
        QFETCH(QString, result);

        builder.addNode(Base::NormalItem {{input}});

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_LineItem_data()
    {
        QTest::addColumn<Base::Line3f>("line");
        QTest::addColumn<Base::DrawStyle>("style");
        QTest::newRow("LineItem") << Base::Line3f {} << Base::DrawStyle {};
    }

    void test_LineItem()
    {
        QFETCH(Base::Line3f, line);
        QFETCH(Base::DrawStyle, style);
        Base::LineItem item {line, style};
        builder.addNode(item);

        SoNode* node = loadBuffer(output.str());
        QVERIFY(node != nullptr);
    }

    void test_MultiLineItem_data()
    {
        QTest::addColumn<Base::Vector3f>("p1");
        QTest::addColumn<Base::Vector3f>("p2");
        QTest::addColumn<Base::Vector3f>("p3");
        QTest::addColumn<Base::DrawStyle>("style");
        QTest::newRow("MultiLineItem") << Base::Vector3f {0, 0, 0} << Base::Vector3f {1, 0, 0}
                                       << Base::Vector3f {1, 1, 0} << Base::DrawStyle {};
    }

    void test_MultiLineItem()
    {
        QFETCH(Base::Vector3f, p1);
        QFETCH(Base::Vector3f, p2);
        QFETCH(Base::Vector3f, p3);
        QFETCH(Base::DrawStyle, style);
        Base::MultiLineItem item {{p1, p2, p3}, style};
        builder.addNode(item);

        SoNode* node = loadBuffer(output.str());
        QVERIFY(node != nullptr);
    }

    void test_ArrowItem_data()
    {
        QTest::addColumn<Base::Line3f>("line");
        QTest::addColumn<Base::DrawStyle>("style");
        QTest::newRow("Arrow") << Base::Line3f {Base::Vector3f {0, 0, 10}, Base::Vector3f {}}
                               << Base::DrawStyle {};
    }

    void test_ArrowItem()
    {
        QFETCH(Base::Line3f, line);
        QFETCH(Base::DrawStyle, style);
        Base::ArrowItem item {line, style};
        builder.addNode(item);

        SoNode* node = loadBuffer(output.str());
        QVERIFY(node != nullptr);
    }

    void test_BoundingBoxItem_data()
    {
        QTest::addColumn<Base::Vector3f>("p1");
        QTest::addColumn<Base::Vector3f>("p2");
        QTest::addColumn<Base::DrawStyle>("style");
        QTest::newRow("BoundingBoxItem")
            << Base::Vector3f {0, 0, 0} << Base::Vector3f {1, 1, 1} << Base::DrawStyle {};
    }

    void test_BoundingBoxItem()
    {
        QFETCH(Base::Vector3f, p1);
        QFETCH(Base::Vector3f, p2);
        QFETCH(Base::DrawStyle, style);
        Base::BoundingBoxItem item {p1, p2, style};
        builder.addNode(item);

        SoNode* node = loadBuffer(output.str());
        QVERIFY(node != nullptr);
    }

    void test_Coordinate3Item_data()
    {
        QTest::addColumn<Base::Vector3f>("p1");
        QTest::addColumn<Base::Vector3f>("p2");
        QTest::addColumn<Base::Vector3f>("p3");
        QTest::newRow("Coordinate3Item")
            << Base::Vector3f {0, 0, 0} << Base::Vector3f {1, 0, 0} << Base::Vector3f {1, 1, 0};
    }

    void test_Coordinate3Item()
    {
        QFETCH(Base::Vector3f, p1);
        QFETCH(Base::Vector3f, p2);
        QFETCH(Base::Vector3f, p3);
        Base::Coordinate3Item item {{p1, p2, p3}};
        builder.addNode(item);

        SoNode* node = loadBuffer(output.str());
        QVERIFY(node != nullptr);
    }

    void test_PointItem_data()
    {
        QTest::addColumn<Base::Vector3f>("point");
        QTest::addColumn<Base::DrawStyle>("style");
        QTest::newRow("PointItem") << Base::Vector3f {} << Base::DrawStyle {};
    }

    void test_PointItem()
    {
        QFETCH(Base::Vector3f, point);
        QFETCH(Base::DrawStyle, style);
        Base::PointItem item {point, style};
        builder.addNode(item);

        SoNode* node = loadBuffer(output.str());
        QVERIFY(node != nullptr);
    }

    void test_NormalBinding_data()
    {
        QTest::addColumn<Base::BindingElement::Binding>("input");
        QTest::addColumn<QString>("result");
        QTest::newRow("NormalBinding")
            << Base::BindingElement::Binding::Overall << "NormalBinding { value OVERALL }\n";
        QTest::newRow("NormalBinding")
            << Base::BindingElement::Binding::PerPart << "NormalBinding { value PER_PART }\n";
        QTest::newRow("NormalBinding") << Base::BindingElement::Binding::PerPartIndexed
                                       << "NormalBinding { value PER_PART_INDEXED }\n";
        QTest::newRow("NormalBinding")
            << Base::BindingElement::Binding::PerFace << "NormalBinding { value PER_FACE }\n";
        QTest::newRow("NormalBinding") << Base::BindingElement::Binding::PerFaceIndexed
                                       << "NormalBinding { value PER_FACE_INDEXED }\n";
        QTest::newRow("NormalBinding")
            << Base::BindingElement::Binding::PerVertex << "NormalBinding { value PER_VERTEX }\n";
        QTest::newRow("NormalBinding") << Base::BindingElement::Binding::PerVertexIndexed
                                       << "NormalBinding { value PER_VERTEX_INDEXED }\n";
        QTest::newRow("NormalBinding")
            << Base::BindingElement::Binding::Default << "NormalBinding { value OVERALL }\n";
        QTest::newRow("NormalBinding")
            << Base::BindingElement::Binding::None << "NormalBinding { value OVERALL }\n";
    }

    void test_NormalBinding()
    {
        QFETCH(Base::BindingElement::Binding, input);
        QFETCH(QString, result);

        Base::NormalBindingItem item;
        item.setValue(input);
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_Cylinder_data()
    {
        auto result =
            R"(Cylinder {
  radius 3
  height 7
  parts (SIDES | TOP | BOTTOM)
}
)";
        QTest::addColumn<float>("radius");
        QTest::addColumn<float>("height");
        QTest::addColumn<QString>("result");
        QTest::newRow("Cylinder") << 3.0F << 7.0F << result;
    }

    void test_Cylinder()
    {
        QFETCH(float, radius);
        QFETCH(float, height);
        QFETCH(QString, result);

        Base::CylinderItem item;
        item.setRadius(radius);
        item.setHeight(height);
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_Cone_data()
    {
        auto result = "Cone { bottomRadius 2 height 10 }\n";

        QTest::addColumn<float>("radius");
        QTest::addColumn<float>("height");
        QTest::addColumn<QString>("result");
        QTest::newRow("Cone") << 2.0F << 10.0F << result;
    }

    void test_Cone()
    {
        QFETCH(float, radius);
        QFETCH(float, height);
        QFETCH(QString, result);

        Base::ConeItem item;
        item.setBottomRadius(radius);
        item.setHeight(height);
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_Sphere_data()
    {
        auto result = "Sphere { radius 4 }\n";

        QTest::addColumn<float>("input");
        QTest::addColumn<QString>("result");
        QTest::newRow("Sphere") << 4.0F << result;
    }

    void test_Sphere()
    {
        QFETCH(float, input);
        QFETCH(QString, result);

        Base::SphereItem item;
        item.setRadius(input);
        builder.addNode(item);

        QString string = QString::fromStdString(output.str());
        QCOMPARE(string, result);
    }

    void test_NurbsSurface_data()
    {
        QTest::addColumn<float>("knot1");
        QTest::addColumn<float>("knot2");
        QTest::addColumn<int>("poles");
        QTest::newRow("Nurbs") << 0.0F << 1.0F << 2;
    }

    void test_NurbsSurface()
    {
        QFETCH(float, knot1);
        QFETCH(float, knot2);
        QFETCH(int, poles);

        Base::NurbsSurfaceItem item;
        item.setKnotVector({knot1, knot2}, {knot1, knot2});
        item.setControlPoints(poles, poles);
        builder.addNode(item);

        SoNode* node = loadBuffer(output.str());
        QVERIFY(node != nullptr);
    }

private:
    std::stringstream output;
    Base::InventorBuilder builder;
};

QTEST_GUILESS_MAIN(testInventorBuilder)

#include "InventorBuilder.moc"
