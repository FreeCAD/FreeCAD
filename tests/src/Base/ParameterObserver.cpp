#include <gtest/gtest.h>
#include <boost/core/ignore_unused.hpp>
#include <Base/ParameterObserver.h>

class ParamObserver: public Base::ParameterObserver
{
public:
    explicit ParamObserver(ParameterGrp::handle hGrp)
    {
        attachToParameter(hGrp);
        setup();
        initParameters();
    }

    bool getBoolParameter() const
    {
        return getValue<bool>("BoolParameter");
    }

    bool getDefaultBoolParameter() const
    {
        return getDefault<bool>("BoolParameter");
    }

    void setBoolParameter(bool v)
    {
        setValue("BoolParameter", v);
    }

    long getLongParameter() const
    {
        return getValue<long>("LongParameter");
    }

    void setLongParameter(long v)
    {
        setValue("LongParameter", v);
    }

    unsigned long getUnsignedParameter() const
    {
        return getValue<unsigned long>("UnsignedParameter");
    }

    void setUnsignedParameter(unsigned long v)
    {
        setValue("UnsignedParameter", v);
    }

    double getFloatParameter() const
    {
        return getValue<double>("FloatParameter");
    }

    void setFloatParameter(double v)
    {
        setValue("FloatParameter", v);
    }

    std::string getStringParameter() const
    {
        return getValue<std::string>("StringParameter");
    }

    void setStringParameter(const std::string& v)
    {
        setValue("StringParameter", v);
    }

    std::string getUnknownParameter() const
    {
        return getValue<std::string>("UnknownParameter");
    }

    void setUnknownParameter(const std::string& v)
    {
        setValue("UnknownParameter", v);
    }

    std::string getWrongParameter() const
    {
        return getValue<std::string>("BoolParameter");
    }

    void setWrongParameter(const std::string& v)
    {
        setValue("BoolParameter", v);
    }

private:
    void setup()
    {
        // NOLINTBEGIN
        addParameter("BoolParameter", Bool{true});
        addParameter("LongParameter", Int{23});
        addParameter("UnsignedParameter", Unsigned{12345});
        addParameter("FloatParameter", Double{1.2345});
        addParameter("StringParameter", String{"Hello"});
        // NOLINTEND
    }
};

class ParameterObserverTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        ParameterManager::Init();
    }

    void SetUp() override
    {
        config = ParameterManager::Create();
    }

    void TearDown() override
    {
    }

    Base::Reference<ParameterManager> getCreateConfig()
    {
        config->CreateDocument();
        return config;
    }

private:
    Base::Reference<ParameterManager> config;
};

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)

TEST_F(ParameterObserverTest, TestBool)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    ParamObserver obs(grp);

    EXPECT_EQ(obs.getBoolParameter(), true);
    obs.setBoolParameter(false);
    EXPECT_EQ(obs.getBoolParameter(), false);

    EXPECT_EQ(grp->GetBool("BoolParameter"), false);
    EXPECT_EQ(obs.getDefaultBoolParameter(), true);
}

TEST_F(ParameterObserverTest, TestInt)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    ParamObserver obs(grp);

    EXPECT_EQ(obs.getLongParameter(), 23);
    obs.setLongParameter(123);
    EXPECT_EQ(obs.getLongParameter(), 123);

    EXPECT_EQ(grp->GetInt("LongParameter"), 123);
}

TEST_F(ParameterObserverTest, TestUnsigned)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    ParamObserver obs(grp);

    EXPECT_EQ(obs.getUnsignedParameter(), 12345);
    obs.setUnsignedParameter(123);
    EXPECT_EQ(obs.getUnsignedParameter(), 123);

    EXPECT_EQ(grp->GetUnsigned("UnsignedParameter"), 123);
}

TEST_F(ParameterObserverTest, TestFloat)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    ParamObserver obs(grp);

    EXPECT_DOUBLE_EQ(obs.getFloatParameter(), 1.2345);
    obs.setFloatParameter(3.14);
    EXPECT_DOUBLE_EQ(obs.getFloatParameter(), 3.14);

    EXPECT_DOUBLE_EQ(grp->GetFloat("FloatParameter"), 3.14);
}

TEST_F(ParameterObserverTest, TestString)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    ParamObserver obs(grp);

    EXPECT_EQ(obs.getStringParameter(), "Hello");
    obs.setStringParameter("World");
    EXPECT_EQ(obs.getStringParameter(), "World");

    EXPECT_EQ(grp->GetASCII("StringParameter"), "World");
}

TEST_F(ParameterObserverTest, TestHashKey)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    ParamObserver obs(grp);

    EXPECT_EQ(obs.getBoolParameter(), true);

    std::string key("BoolParameter");
    grp->SetBool(key.c_str(), false);

    EXPECT_EQ(obs.getBoolParameter(), false);
}

TEST_F(ParameterObserverTest, TestUnknown)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    ParamObserver obs(grp);

    EXPECT_THROW(obs.getUnknownParameter(), Base::IndexError);
    EXPECT_THROW(obs.setUnknownParameter("123"), Base::IndexError);
}

TEST_F(ParameterObserverTest, TestWrongType)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    ParamObserver obs(grp);

    EXPECT_THROW(obs.getWrongParameter(), Base::TypeError);
    EXPECT_THROW(obs.setWrongParameter("123"), Base::TypeError);
}

// NOLINTEND(cppcoreguidelines-*,readability-*)
