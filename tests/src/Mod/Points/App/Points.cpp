#include "gtest/gtest.h"
#include <Base/FileInfo.h>
#include <Mod/Points/App/Points.h>
#include <Mod/Points/App/PointsAlgos.h>

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)

class PointsTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        std::vector<Base::Vector3f> points;
        points.emplace_back(0, 0, 0);
        points.emplace_back(0, 0, 1);
        points.emplace_back(0, 1, 0);
        points.emplace_back(0, 1, 1);
        points.emplace_back(1, 0, 0);
        points.emplace_back(1, 0, 1);
        points.emplace_back(1, 1, 0);
        points.emplace_back(1, 1, 1);
        kernel.setBasicPoints(points);

        tmp.setFile(Base::FileInfo::getTempFileName());
    }

    void TearDown() override
    {
        tmp.deleteFile();
    }

    const Points::PointKernel& getKernel() const
    {
        return kernel;
    }
    std::string getFileName() const
    {
        return tmp.filePath();
    }
    std::vector<float> getIntensity() const
    {
        return {0.1F, 0.2F, 0.3F, 0.4F, 0.3F, 0.2F, 0.3F, 0.1F};
    }
    std::vector<Base::Vector3f> getNormals() const
    {
        std::vector<Base::Vector3f> vec(8, Base::Vector3f(0, 0, 1));
        return vec;
    }
    std::vector<App::Color> getColors() const
    {
        std::vector<App::Color> col(8);
        col[0].set(0, 0, 0);
        col[1].set(0, 0, 1);
        col[2].set(0, 1, 0);
        col[3].set(0, 1, 1);
        col[4].set(1, 0, 0);
        col[5].set(1, 0, 1);
        col[6].set(1, 1, 0);
        col[7].set(1, 1, 1);
        return col;
    }

private:
    Points::PointKernel kernel;
    Base::FileInfo tmp;
};

TEST_F(PointsTest, TestDefault)
{
    Points::PointKernel kernel;
    std::vector<Points::PointKernel::value_type> points;
    kernel.setBasicPoints(points);
    EXPECT_EQ(kernel.size(), 0);
}

TEST_F(PointsTest, TestSize)
{
    Points::PointKernel kernel(20);
    EXPECT_EQ(kernel.size(), 20);
}

TEST_F(PointsTest, TestCopy)
{
    Points::PointKernel kernel(20);
    Points::PointKernel copy(kernel);  // NOLINT
    EXPECT_EQ(copy.size(), 20);
}

TEST_F(PointsTest, TestMove)
{
    Points::PointKernel kernel(20);
    Points::PointKernel move(std::move(kernel));
    EXPECT_EQ(move.size(), 20);
}

TEST_F(PointsTest, TestAssign)
{
    Points::PointKernel kernel(20);
    Points::PointKernel copy;
    copy = kernel;
    EXPECT_EQ(copy.size(), 20);
}

TEST_F(PointsTest, TestMoveAssign)
{
    Points::PointKernel kernel(20);
    Points::PointKernel copy;
    copy = std::move(kernel);
    EXPECT_EQ(copy.size(), 20);
}

TEST_F(PointsTest, TestSwap)
{
    Points::PointKernel kernel;
    std::vector<Points::PointKernel::value_type> points(20);
    kernel.swap(points);
    EXPECT_EQ(kernel.size(), 20);
    EXPECT_EQ(points.size(), 0);
    EXPECT_EQ(kernel.countValid(), 20);
}

TEST_F(PointsTest, TestASCII)
{
    std::string name = getFileName() + ".asc";
    Points::AscWriter writer(getKernel());
    writer.setIntensities(getIntensity());
    writer.write(name);

    Points::AscReader reader;
    reader.read(name);

    EXPECT_FALSE(reader.hasProperties());
    EXPECT_FALSE(reader.hasIntensities());
    EXPECT_FALSE(reader.hasColors());
    EXPECT_FALSE(reader.hasNormals());
    EXPECT_FALSE(reader.isStructured());
    EXPECT_EQ(reader.getWidth(), 8);
    EXPECT_EQ(reader.getHeight(), 1);
}

TEST_F(PointsTest, TestPlainPLY)
{
    std::string name = getFileName();
    Points::PlyWriter writer(getKernel());
    writer.write(name);

    Points::PlyReader reader;
    reader.read(name);

    EXPECT_FALSE(reader.hasProperties());
    EXPECT_FALSE(reader.hasIntensities());
    EXPECT_FALSE(reader.hasColors());
    EXPECT_FALSE(reader.hasNormals());
    EXPECT_FALSE(reader.isStructured());
    EXPECT_EQ(reader.getWidth(), 8);
    EXPECT_EQ(reader.getHeight(), 1);
}

TEST_F(PointsTest, TestPLYWithProperties)
{
    std::string name = getFileName();
    Points::PlyWriter writer(getKernel());
    writer.setIntensities(getIntensity());
    writer.setColors(getColors());
    writer.setNormals(getNormals());
    writer.write(name);

    Points::PlyReader reader;
    reader.read(name);

    EXPECT_TRUE(reader.hasProperties());
    EXPECT_TRUE(reader.hasIntensities());
    EXPECT_TRUE(reader.hasColors());
    EXPECT_TRUE(reader.hasNormals());
    EXPECT_FALSE(reader.isStructured());
    EXPECT_EQ(reader.getWidth(), 8);
    EXPECT_EQ(reader.getHeight(), 1);
}

TEST_F(PointsTest, TestPlainPCD)
{
    std::string name = getFileName();
    Points::PcdWriter writer(getKernel());
    writer.write(name);

    Points::PcdReader reader;
    reader.read(name);

    EXPECT_FALSE(reader.hasProperties());
    EXPECT_FALSE(reader.hasIntensities());
    EXPECT_FALSE(reader.hasColors());
    EXPECT_FALSE(reader.hasNormals());
    EXPECT_FALSE(reader.isStructured());
    EXPECT_EQ(reader.getWidth(), 8);
    EXPECT_EQ(reader.getHeight(), 1);
}

TEST_F(PointsTest, TestPCDWithProperties)
{
    std::string name = getFileName();
    Points::PcdWriter writer(getKernel());
    writer.setIntensities(getIntensity());
    writer.setColors(getColors());
    writer.setNormals(getNormals());
    writer.write(name);

    Points::PcdReader reader;
    reader.read(name);

    EXPECT_TRUE(reader.hasProperties());
    EXPECT_TRUE(reader.hasIntensities());
    EXPECT_TRUE(reader.hasColors());
    EXPECT_TRUE(reader.hasNormals());
    EXPECT_FALSE(reader.isStructured());
    EXPECT_EQ(reader.getWidth(), 8);
    EXPECT_EQ(reader.getHeight(), 1);
}

TEST_F(PointsTest, TestPCDStructured)
{
    std::string name = getFileName();
    Points::PcdWriter writer(getKernel());
    writer.setIntensities(getIntensity());
    writer.setColors(getColors());
    writer.setNormals(getNormals());
    writer.setWidth(4);
    writer.setHeight(2);
    writer.write(name);

    Points::PcdReader reader;
    reader.read(name);

    EXPECT_TRUE(reader.hasProperties());
    EXPECT_TRUE(reader.hasIntensities());
    EXPECT_TRUE(reader.hasColors());
    EXPECT_TRUE(reader.hasNormals());
    EXPECT_TRUE(reader.isStructured());
    EXPECT_EQ(reader.getWidth(), 4);
    EXPECT_EQ(reader.getHeight(), 2);
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
