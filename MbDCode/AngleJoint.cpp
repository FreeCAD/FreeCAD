#include "AngleJoint.h"
#include "CREATE.h"
#include "System.h"
#include "DirectionCosineConstraintIJ.h"

using namespace MbD;

MbD::AngleJoint::AngleJoint()
{
}

MbD::AngleJoint::AngleJoint(const char* str) : Joint(str)
{
}

void MbD::AngleJoint::initializeGlobally()
{
	if (constraints->empty())
	{
		auto dirCosIzJz = CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 2, 2);
		dirCosIzJz->setConstant(std::cos(theIzJz));
		addConstraint(dirCosIzJz);
		this->root()->hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}
