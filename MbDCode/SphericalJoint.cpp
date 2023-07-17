#include "SphericalJoint.h"
#include "CREATE.h"
#include "System.h"

using namespace MbD;

MbD::SphericalJoint::SphericalJoint()
{
}

MbD::SphericalJoint::SphericalJoint(const char* str) : AtPointJoint(str)
{
}

void MbD::SphericalJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		createAtPointConstraints();
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
