#include "PointInPlaneJoint.h"
#include "System.h"

using namespace MbD;

MbD::PointInPlaneJoint::PointInPlaneJoint()
{
}

MbD::PointInPlaneJoint::PointInPlaneJoint(const char* str)
{
}

void MbD::PointInPlaneJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		this->createInPlaneConstraint();
		this->root()->hasChanged = true;
	}
	else {
		InPlaneJoint::initializeGlobally();
	}
}
