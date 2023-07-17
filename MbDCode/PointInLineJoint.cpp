#include "PointInLineJoint.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::PointInLineJoint::PointInLineJoint()
{
}

MbD::PointInLineJoint::PointInLineJoint(const char* str) : InLineJoint(str)
{
}

void MbD::PointInLineJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		createInLineConstraints();
		this->root()->hasChanged = true;
	}
	else {
		InLineJoint::initializeGlobally();
	}
}
