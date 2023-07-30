#include "ASMTCylindricalJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTCylindricalJoint::mbdClassNew()
{
    return CREATE<CylindricalJoint>::With();
}
