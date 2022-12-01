#include <QTest>
#include <sstream>
#include <Base/Builder3D.h>

class testInventorBuilder : public QObject
{
    Q_OBJECT

public:
    testInventorBuilder()
        : builder(output)
    {
    }
    ~testInventorBuilder()
    {
    }

private Q_SLOTS:
    void initTestCase()
    {

    }
    void initTestCase_data()
    {

    }
    void cleanupTestCase()
    {

    }

    void init()
    {
    }

    void cleanup()
    {
        // clear the buffer
        output.str(std::string());
    }

    void test_Output()
    {
        QCOMPARE(output.str().c_str(), "#Inventor V2.1 ascii \n\n");
    }

    void test_MaterialBinding()
    {
        Base::MaterialBindingItem item{Base::MaterialBinding{}};
        builder.addNode(item);

        QCOMPARE(output.str().c_str(), "MaterialBinding { value OVERALL } \n");
    }

private:
    std::stringstream output;
    Base::InventorBuilder builder;
};

QTEST_GUILESS_MAIN(testInventorBuilder)

#include "InventorBuilder.moc"
