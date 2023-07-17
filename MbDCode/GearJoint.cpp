#include "GearJoint.h"
#include "CREATE.h"
#include "GearConstraintIJ.h"
#include "System.h"

using namespace MbD;

MbD::GearJoint::GearJoint()
{
}

MbD::GearJoint::GearJoint(const char* str)
{
}

void MbD::GearJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		auto gearIJ = CREATE<GearConstraintIJ>::With(frmI, frmJ);
		gearIJ->radiusI = radiusI;
		gearIJ->radiusJ = radiusJ;
		gearIJ->setConstant(aConstant);
		addConstraint(gearIJ);
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
