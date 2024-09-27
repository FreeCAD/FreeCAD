#include <gtest/gtest.h>

#include "App/PropertyLinks.h"
#include <App/PropertyStandard.h>
#include <Base/Writer.h>
#include <Base/Reader.h>

TEST(PropertyLink, TestSetValues)
{
    App::PropertyLinkSubList prop;
    std::vector<App::DocumentObject*> objs {nullptr, nullptr};
    std::vector<const char*> subs {"Sub1", "Sub2"};
    prop.setValues(objs, subs);
    const auto& sub = prop.getSubValues();
    EXPECT_EQ(sub.size(), 2);
    EXPECT_EQ(sub[0], "Sub1");
    EXPECT_EQ(sub[1], "Sub2");
}

TEST(PropertyFloatTest, testWriteRead)
{
#if defined(FC_OS_LINUX) || defined(FC_OS_BSD)
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");  // avoid rounding of floating point numbers
#endif
    double value = 1.2345;
    App::PropertyFloat prop;
    prop.setValue(value);
    Base::StringWriter writer;
    prop.Save(writer);

    std::string str = "<?xml version='1.0' encoding='utf-8'?>\n";
    str.append("<Property name='Length' type='App::PropertyFloat'>\n");
    str.append(writer.getString());
    str.append("</Property>\n");

    std::stringstream data(str);
    Base::XMLReader reader("Document.xml", data);
    App::PropertyFloat prop2;
    prop2.Restore(reader);
    EXPECT_DOUBLE_EQ(prop2.getValue(), value);
}
