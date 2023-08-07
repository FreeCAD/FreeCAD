#include "ASMTFixedJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTFixedJoint::mbdClassNew()
{
    return CREATE<FixedJoint>::With();
}
