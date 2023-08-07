#include "ASMTSphericalJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTSphericalJoint::mbdClassNew()
{
    return CREATE<SphericalJoint>::With();
}
