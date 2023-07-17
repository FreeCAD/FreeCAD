#include "CylSphJoint.h"
#include "CREATE.h"
#include "DistancexyConstraintIJ.h"
#include "System.h"

using namespace MbD;

MbD::CylSphJoint::CylSphJoint()
{
}

MbD::CylSphJoint::CylSphJoint(const char* str) : CompoundJoint(str)
{
}

void MbD::CylSphJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		auto distxyIJ = CREATE<DistancexyConstraintIJ>::With(frmI, frmJ);
		distxyIJ->setConstant(distanceIJ);
		addConstraint(distxyIJ);
		this->root()->hasChanged = true;
	}
	else {
		CompoundJoint::initializeGlobally();
	}
}
