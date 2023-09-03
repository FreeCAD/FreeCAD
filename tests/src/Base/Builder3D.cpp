#include "gtest/gtest.h"

#include <Base/Builder3D.h>

TEST(Builder3D, openInventor)
{
    Base::Builder3D builder;
    builder.beginSeparator();
    builder.endSeparator();
}
