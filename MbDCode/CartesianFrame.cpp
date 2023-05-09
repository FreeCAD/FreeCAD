#include "CartesianFrame.h"

using namespace MbD;

CartesianFrame::CartesianFrame()
{
}

CartesianFrame::CartesianFrame(const char* str) : Item(str)
{
}

void CartesianFrame::initialize()
{
	Item::initialize();
}
