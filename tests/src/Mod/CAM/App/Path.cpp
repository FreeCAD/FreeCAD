#include <numbers>

#include <gtest/gtest.h>
#include <Base/Exception.h>
#include <Mod/CAM/App/Command.h>
#include <Mod/CAM/App/Path.h>

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
TEST(PathTest, deleteCommand1)
{
    Path::Toolpath path;
    EXPECT_THROW(path.deleteCommand(0), Base::IndexError);
}

TEST(PathTest, deleteCommand2)
{
    Path::Toolpath path;
    EXPECT_THROW(path.deleteCommand(-1), Base::IndexError);
}

TEST(PathTest, deleteCommand3)
{
    Path::Toolpath path;
    EXPECT_THROW(path.deleteCommand(-2), Base::IndexError);
}

TEST(PathTest, deleteCommand4)
{
    Path::Toolpath path;
    Path::Command cmd;
    path.addCommand(cmd);
    path.deleteCommand(0);
    EXPECT_TRUE(true);
}

TEST(PathTest, deleteCommand5)
{
    Path::Toolpath path;
    Path::Command cmd;
    path.addCommand(cmd);
    path.deleteCommand(-1);
    EXPECT_TRUE(true);
}

TEST(PathTest, deleteCommand6)
{
    Path::Toolpath path;
    Path::Command cmd;
    path.addCommand(cmd);
    EXPECT_THROW(path.deleteCommand(-2), Base::IndexError);
}

TEST(PathTest, insertCommand1)
{
    Path::Toolpath path;
    Path::Command cmd;
    path.insertCommand(cmd, 0);
    EXPECT_TRUE(true);
}

TEST(PathTest, insertCommand2)
{
    Path::Toolpath path;
    Path::Command cmd;
    EXPECT_THROW(path.insertCommand(cmd, 1), Base::IndexError);
}

TEST(PathTest, insertCommand3)
{
    Path::Toolpath path;
    Path::Command cmd;
    path.insertCommand(cmd, -1);
    EXPECT_TRUE(true);
}

TEST(PathTest, insertCommand4)
{
    Path::Toolpath path;
    Path::Command cmd;
    EXPECT_THROW(path.insertCommand(cmd, -2), Base::IndexError);
}

TEST(PathTest, getLength1)
{
    Path::Toolpath path;
    Path::Command cmd;
    path.addCommand(cmd);
    EXPECT_DOUBLE_EQ(path.getLength(), 0.0);
}

// For g-code examples have a look at https://linuxcnc.org/docs/html/gcode/g-code.html

TEST(PathTest, getLength2)
{
    Path::Toolpath path;
    Path::Command cmd;
    cmd.setFromGCode("G90");
    path.addCommand(cmd);
    cmd.setFromGCode("G0 X3 Y-4.0");
    path.addCommand(cmd);
    cmd.setFromGCode("M2");
    path.addCommand(cmd);
    EXPECT_DOUBLE_EQ(path.getLength(), 5.0);
}

TEST(PathTest, getLength3)
{
    Path::Toolpath path;
    Path::Command cmd;
    cmd.setFromGCode("G90");
    path.addCommand(cmd);
    cmd.setFromGCode("G1 X3 Y-4.0 F10");
    path.addCommand(cmd);
    cmd.setFromGCode("G0 Z-2.5");
    path.addCommand(cmd);
    cmd.setFromGCode("G0 Z1.0");
    path.addCommand(cmd);
    cmd.setFromGCode("M2");
    path.addCommand(cmd);
    EXPECT_DOUBLE_EQ(path.getLength(), 11.0);
}

TEST(PathTest, getLength4)
{
    Path::Toolpath path;
    Path::Command cmd;
    cmd.setFromGCode("G0 X2 Y0");
    path.addCommand(cmd);
    cmd.setFromGCode("G3 X3 Y1 I0 J1 F10");
    path.addCommand(cmd);
    EXPECT_NEAR(path.getLength(), 2.0 + std::numbers::pi / 2, 1e-12);
}

TEST(PathTest, getLength5)
{
    Path::Toolpath path;
    Path::Command cmd;
    cmd.setFromGCode("G0 X2 Y0");
    path.addCommand(cmd);
    cmd.setFromGCode("G2 X3 Y1 I0 J1 F10");
    path.addCommand(cmd);
    EXPECT_NEAR(path.getLength(), 2.0 + std::numbers::pi * 3 / 2, 1e-12);
}

TEST(PathTest, getLength6)
{
    Path::Toolpath path;
    Path::Command cmd;
    cmd.setFromGCode("G0 X2 Y0");
    path.addCommand(cmd);
    cmd.setFromGCode("G2 X2 Y0 I1 J0 F10");
    path.addCommand(cmd);
    EXPECT_NEAR(path.getLength(), 2.0 + std::numbers::pi * 2, 1e-12);
}

TEST(PathTest, getLength7)
{
    Path::Toolpath path;
    Path::Command cmd;
    cmd.setFromGCode("G0 X2 Y0");
    path.addCommand(cmd);
    cmd.setFromGCode("G90.1");
    path.addCommand(cmd);
    cmd.setFromGCode("G3 X3 Y1 I2 J1 F10");
    path.addCommand(cmd);
    cmd.setFromGCode("G91.1");
    path.addCommand(cmd);
    cmd.setFromGCode("G3 X4 Y2 I0 J1 F10");
    path.addCommand(cmd);
    EXPECT_NEAR(path.getLength(), 2.0 + std::numbers::pi, 1e-12);
}

TEST(PathTest, getCycleTime)
{
    Path::Toolpath path;
    Path::Command cmd;
    cmd.setFromGCode("G0 X2 Y0");
    path.addCommand(cmd);
    cmd.setFromGCode("G2 X3 Y1 I0 J1 F10");
    path.addCommand(cmd);
    EXPECT_NEAR(path.getCycleTime(1.0, 1.0, 1.0, 1.0), 2.0 + std::numbers::pi * 3 / 2, 1e-12);
}

TEST(PathTest, assign)
{
    Path::Toolpath path;
    Path::Command cmd;
    cmd.setFromGCode("G0 X2 Y0");
    path.addCommand(cmd);
    cmd.setFromGCode("G3 X3 Y1 I0 J1 F10");
    path.addCommand(cmd);

    Path::Toolpath path2;
    path2 = path;
    EXPECT_NEAR(path.getLength(), 2.0 + std::numbers::pi / 2, 1e-12);
    EXPECT_NEAR(path2.getLength(), 2.0 + std::numbers::pi / 2, 1e-12);
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
