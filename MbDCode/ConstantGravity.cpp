#include "ConstantGravity.h"
#include "System.h"

using namespace MbD;

void MbD::ConstantGravity::submitToSystem(std::shared_ptr<Item> self)
{
	root()->forcesTorques->push_back(std::static_pointer_cast<ForceTorqueItem>(self));
}
