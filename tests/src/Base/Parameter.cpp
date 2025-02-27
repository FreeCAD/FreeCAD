#include <gtest/gtest.h>
#include <boost/core/ignore_unused.hpp>
#include <QLockFile>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>

class FakeObserver: public ParameterGrp::ObserverType
{
public:
    void clearNotifications()
    {
        notify = 0;
    }
    int getCountNotifications() const
    {
        return notify;
    }
    void attachSelf(ParameterGrp::handle hGrp)
    {
        hGrp->Attach(this);
    }
    void detachSelf(ParameterGrp::handle hGrp)
    {
        hGrp->Detach(this);
    }
    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override
    {
        boost::ignore_unused(rCaller);
        boost::ignore_unused(Reason);
        notify++;
    }

private:
    int notify {};
};

class ParameterTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        ParameterManager::Init();
    }

    ParameterTest()
    {
        fi.setFile(Base::FileInfo::getTempFileName() + ".cfg");
    }

    void SetUp() override
    {
        config = ParameterManager::Create();
    }

    void TearDown() override
    {
        if (fi.exists()) {
            fi.deleteFile();
        }
    }

    Base::Reference<ParameterManager> getConfig() const
    {
        return config;
    }

    Base::Reference<ParameterManager> getCreateConfig()
    {
        config->CreateDocument();
        return config;
    }

    std::string getFileName() const
    {
        return fi.filePath();
    }

    FakeObserver& getObserver()
    {
        return observer;
    }

private:
    Base::FileInfo fi;
    FakeObserver observer;
    Base::Reference<ParameterManager> config;
};

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
TEST_F(ParameterTest, TestValid)
{
    auto cfg = getConfig();
    EXPECT_EQ(cfg.isValid(), true);
    EXPECT_EQ(cfg.isNull(), false);
}

TEST_F(ParameterTest, TestCreate)
{
    auto cfg = getCreateConfig();
    cfg->CheckDocument();
    EXPECT_TRUE(cfg->IsEmpty());
}

TEST_F(ParameterTest, TestGroup)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    EXPECT_EQ(grp->Parent(), static_cast<ParameterGrp*>(cfg));
    EXPECT_EQ(grp->Manager(), static_cast<ParameterGrp*>(cfg));

    EXPECT_FALSE(cfg->IsEmpty());
    EXPECT_TRUE(grp->IsEmpty());

    EXPECT_TRUE(cfg->HasGroup("TopLevelGroup"));
    EXPECT_FALSE(cfg->HasGroup("Group"));

    EXPECT_EQ(cfg->GetGroups().size(), 1);
}

TEST_F(ParameterTest, TestGroupName)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    EXPECT_STREQ(grp->GetGroupName(), "TopLevelGroup");
}

TEST_F(ParameterTest, TestEmptyGroupName)
{
    auto cfg = getCreateConfig();
    EXPECT_THROW(cfg->GetGroup(nullptr), Base::ValueError);
    EXPECT_THROW(cfg->GetGroup(""), Base::ValueError);
    EXPECT_THROW(cfg->GetGroup("///////"), Base::ValueError);
}

TEST_F(ParameterTest, TestGroupNames)
{
    auto cfg = getCreateConfig();
    auto grp1 = cfg->GetGroup("////Sub1/////Sub2/////");
    auto grp2 = cfg->GetGroup("Sub1/Sub2");
    EXPECT_STREQ(grp1->GetGroupName(), "Sub2");
    EXPECT_STREQ(grp2->GetGroupName(), "Sub2");
}

TEST_F(ParameterTest, TestPath)
{
    auto cfg = getCreateConfig();
    auto grp1 = cfg->GetGroup("TopLevelGroup");
    auto sub1 = grp1->GetGroup("Sub1");
    auto sub2 = sub1->GetGroup("Sub2");
    EXPECT_EQ(sub2->GetPath(), "TopLevelGroup/Sub1/Sub2");
}

TEST_F(ParameterTest, TestBool)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    grp->SetBool("Parameter1", false);
    grp->SetBool("Parameter2", true);
    EXPECT_EQ(grp->GetBool("Parameter1", true), false);
    EXPECT_EQ(grp->GetBool("Parameter3", true), true);
    EXPECT_EQ(grp->GetBool("Parameter3", false), false);

    EXPECT_TRUE(grp->GetBools("Test").empty());
    EXPECT_EQ(grp->GetBools().size(), 2);
    EXPECT_EQ(grp->GetBools().at(0), false);
    EXPECT_EQ(grp->GetBools().at(1), true);
    EXPECT_EQ(grp->GetBoolMap().size(), 2);

    grp->RemoveBool("Parameter1");
    EXPECT_EQ(grp->GetBools().size(), 1);
}

TEST_F(ParameterTest, TestInt)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    grp->SetInt("Parameter1", -15);
    grp->SetInt("Parameter2", 25);
    EXPECT_EQ(grp->GetInt("Parameter1", 2), -15);
    EXPECT_EQ(grp->GetInt("Parameter3", 2), 2);
    EXPECT_EQ(grp->GetInt("Parameter3", 4), 4);

    EXPECT_TRUE(grp->GetInts("Test").empty());
    EXPECT_EQ(grp->GetInts().size(), 2);
    EXPECT_EQ(grp->GetInts().at(0), -15);
    EXPECT_EQ(grp->GetInts().at(1), 25);
    EXPECT_EQ(grp->GetIntMap().size(), 2);

    grp->RemoveInt("Parameter1");
    EXPECT_EQ(grp->GetInts().size(), 1);
}

TEST_F(ParameterTest, TestUnsigned)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    grp->SetUnsigned("Parameter1", 15);
    grp->SetUnsigned("Parameter2", 25);
    EXPECT_EQ(grp->GetUnsigned("Parameter1", 2), 15);
    EXPECT_EQ(grp->GetUnsigned("Parameter3", 2), 2);
    EXPECT_EQ(grp->GetUnsigned("Parameter3", 4), 4);

    EXPECT_TRUE(grp->GetUnsigneds("Test").empty());
    EXPECT_EQ(grp->GetUnsigneds().size(), 2);
    EXPECT_EQ(grp->GetUnsigneds().at(0), 15);
    EXPECT_EQ(grp->GetUnsigneds().at(1), 25);
    EXPECT_EQ(grp->GetUnsignedMap().size(), 2);

    grp->RemoveUnsigned("Parameter1");
    EXPECT_EQ(grp->GetUnsigneds().size(), 1);
}

TEST_F(ParameterTest, TestFloat)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    grp->SetFloat("Parameter1", 1.5);
    grp->SetFloat("Parameter2", 2.5);
    EXPECT_EQ(grp->GetFloat("Parameter1", 2.0), 1.5);
    EXPECT_EQ(grp->GetFloat("Parameter3", 2.0), 2.0);
    EXPECT_EQ(grp->GetFloat("Parameter3", 4.0), 4.0);

    EXPECT_TRUE(grp->GetFloats("Test").empty());
    EXPECT_EQ(grp->GetFloats().size(), 2);
    EXPECT_EQ(grp->GetFloats().at(0), 1.5);
    EXPECT_EQ(grp->GetFloats().at(1), 2.5);
    EXPECT_EQ(grp->GetFloatMap().size(), 2);

    grp->RemoveFloat("Parameter1");
    EXPECT_EQ(grp->GetFloats().size(), 1);
}

TEST_F(ParameterTest, TestString)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    grp->SetASCII("Parameter1", "Value1");
    grp->SetASCII("Parameter2", "Value2");
    EXPECT_EQ(grp->GetASCII("Parameter1", "Value3"), "Value1");
    EXPECT_EQ(grp->GetASCII("Parameter3", "Value3"), "Value3");
    EXPECT_EQ(grp->GetASCII("Parameter3", "Value4"), "Value4");

    EXPECT_TRUE(grp->GetASCIIs("Test").empty());
    EXPECT_EQ(grp->GetASCIIs().size(), 2);
    EXPECT_EQ(grp->GetASCIIs().at(0), "Value1");
    EXPECT_EQ(grp->GetASCIIs().at(1), "Value2");
    EXPECT_EQ(grp->GetASCIIMap().size(), 2);

    grp->RemoveASCII("Parameter1");
    EXPECT_EQ(grp->GetASCIIs().size(), 1);
}

TEST_F(ParameterTest, TestCopy)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    auto sub2 = grp->GetGroup("Sub1/Sub2");
    sub2->SetFloat("Parameter", 1.5);

    auto sub3 = grp->GetGroup("Sub3");
    sub3->SetFloat("AnotherParameter", 2.5);
    sub2->copyTo(sub3);
    EXPECT_TRUE(sub3->GetFloats("Test").empty());
    EXPECT_EQ(sub3->GetFloats().size(), 1);
    EXPECT_EQ(sub3->GetFloats().at(0), 1.5);

    // Test that old parameter has been removed
    EXPECT_TRUE(sub3->GetFloats("AnotherParameter").empty());
}

TEST_F(ParameterTest, TestInsert)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    auto sub2 = grp->GetGroup("Sub1/Sub2");
    sub2->SetFloat("Parameter", 1.5);

    auto sub3 = grp->GetGroup("Sub3");
    sub3->SetFloat("AnotherParameter", 2.5);
    sub2->insertTo(sub3);

    EXPECT_EQ(sub3->GetFloats().size(), 2);
    EXPECT_EQ(sub3->GetFloats("AnotherParameter").size(), 1);
}

TEST_F(ParameterTest, TestRevert)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    auto sub1 = grp->GetGroup("Sub1/Sub/Sub");
    sub1->SetFloat("Float", 1.5);

    auto sub2 = grp->GetGroup("Sub2/Sub/Sub");
    sub2->SetFloat("Float", 1.5);

    grp->GetGroup("Sub1")->revert(grp->GetGroup("Sub2"));

    EXPECT_EQ(sub1->GetFloat("Float", 0.0), 0.0);
    EXPECT_EQ(sub1->GetFloat("Float", 2.0), 2.0);
    EXPECT_EQ(sub2->GetFloat("Float", 2.0), 1.5);
}

TEST_F(ParameterTest, TestRemoveGroup)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    auto sub1 = grp->GetGroup("Sub1");
    auto sub2 = sub1->GetGroup("Sub2/Sub/Sub");
    sub2->SetFloat("Float", 1.5);
    sub1->RemoveGrp("Sub2");
    sub2->SetInt("Int", 2);
    EXPECT_EQ(sub2->GetInt("Int", 0), 2);
    EXPECT_EQ(sub2->GetInt("Int", 1), 2);
    cfg->CheckDocument();
}

TEST_F(ParameterTest, TestRenameGroup)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup");
    auto sub1 = grp->GetGroup("Sub1");
    auto sub2 = sub1->GetGroup("Sub2/Sub/Sub");
    sub2->SetFloat("Float", 1.5);
    sub1->RenameGrp("Sub2", "Sub3");
    sub2->SetInt("Int", 2);
    EXPECT_EQ(sub2->GetInt("Int", 0), 2);
    EXPECT_EQ(sub2->GetInt("Int", 1), 2);
    cfg->CheckDocument();
}

TEST_F(ParameterTest, TestSaveRestoreRef)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup/Sub1/Sub2");
    grp->SetFloat("Float", 1.0);
    cfg->CheckDocument();

    std::string fn = getFileName();
    cfg->exportTo(fn.c_str());

    cfg->importFrom(fn.c_str());
    auto grp2 = cfg->GetGroup("TopLevelGroup/Sub1/Sub2");
    EXPECT_EQ(grp, grp2);

    grp2->SetFloat("Float", 2.0);
    cfg->exportTo(fn.c_str());
}

TEST_F(ParameterTest, TestSaveRestoreNoRef)
{
    auto cfg = getCreateConfig();
    auto grp = cfg->GetGroup("TopLevelGroup/Sub1/Sub2");
    grp->SetFloat("Float", 1.0);
    grp = nullptr;
    cfg->CheckDocument();

    std::string fn = getFileName();
    cfg->exportTo(fn.c_str());

    cfg->importFrom(fn.c_str());
    auto grp2 = cfg->GetGroup("TopLevelGroup/Sub1/Sub2");
    EXPECT_NE(grp, grp2);

    grp2->SetFloat("Float", 2.0);
    cfg->exportTo(fn.c_str());
}

TEST_F(ParameterTest, TestGroupRef)
{
    auto cfg = getCreateConfig();
    cfg->GetGroup("TopLevelGroup");

    std::string fn = getFileName();
    cfg->exportTo(fn.c_str());

    // keep reference to prevent the deletion of the group node
    auto grp = cfg->GetGroup("TopLevelGroup/Sub1/Sub2");
    cfg->importFrom(fn.c_str());
    auto top = cfg->GetGroup("TopLevelGroup");
    EXPECT_TRUE(top->HasGroup("Sub1"));
}

TEST_F(ParameterTest, TestGroupNoRef)
{
    auto cfg = getCreateConfig();
    cfg->GetGroup("TopLevelGroup");

    std::string fn = getFileName();
    cfg->exportTo(fn.c_str());

    // nullify reference to delete the group node
    auto grp = cfg->GetGroup("TopLevelGroup/Sub1/Sub2");
    grp = nullptr;

    cfg->importFrom(fn.c_str());
    auto top = cfg->GetGroup("TopLevelGroup");
    EXPECT_FALSE(top->HasGroup("Sub1"));
}

TEST_F(ParameterTest, TestObserverRef)
{
    auto cfg = getCreateConfig();
    cfg->GetGroup("TopLevelGroup");

    std::string fn = getFileName();
    cfg->exportTo(fn.c_str());

    auto grp = cfg->GetGroup("TopLevelGroup/Sub1/Sub2");

    auto& obs = getObserver();
    obs.attachSelf(grp);

    grp->SetFloat("Float", 1.0);

    EXPECT_EQ(obs.getCountNotifications(), 1);
    obs.clearNotifications();

    cfg->importFrom(fn.c_str());

    grp->SetFloat("Float", 2.0);
    EXPECT_EQ(obs.getCountNotifications(), 3);
    obs.detachSelf(grp);
}

TEST_F(ParameterTest, TestObserverNoRef)
{
    auto cfg = getCreateConfig();
    cfg->GetGroup("TopLevelGroup");

    std::string fn = getFileName();
    cfg->exportTo(fn.c_str());

    auto grp = cfg->GetGroup("TopLevelGroup/Sub1/Sub2");

    auto& obs = getObserver();
    obs.attachSelf(grp);
    grp = nullptr;

    cfg->importFrom(fn.c_str());

    auto grp2 = cfg->GetGroup("TopLevelGroup/Sub1/Sub2");
    grp2->SetFloat("Float", 2.0);
    EXPECT_EQ(obs.getCountNotifications(), 1);
}

TEST_F(ParameterTest, TestLockFile)
{
    std::string fn = getFileName();
    fn.append(".lock");

    QLockFile lockFile1(QString::fromStdString(fn));
    EXPECT_TRUE(lockFile1.tryLock(100));
    EXPECT_TRUE(lockFile1.isLocked());

    QLockFile lockFile2(QString::fromStdString(fn));
    EXPECT_FALSE(lockFile2.tryLock(100));
    EXPECT_FALSE(lockFile2.isLocked());

    lockFile1.unlock();
    EXPECT_TRUE(lockFile2.lock());
    EXPECT_FALSE(lockFile1.tryLock(500));
    lockFile2.unlock();
}

// NOLINTEND(cppcoreguidelines-*,readability-*)
