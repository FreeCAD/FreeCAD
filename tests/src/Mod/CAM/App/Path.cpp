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
    path.deleteCommand(-1);
    EXPECT_TRUE(true);
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
    cmd.setFromGCode("G0 X0 Y0");
    path.addCommand(cmd);
    cmd.setFromGCode("G2 X1 Y1 I1 F10");
    path.addCommand(cmd);
    EXPECT_GT(path.getLength(), 1.57);  // PI/2
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
