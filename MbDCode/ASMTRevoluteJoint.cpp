#include "ASMTRevoluteJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTRevoluteJoint::mbdClassNew()
{
    return CREATE<RevoluteJoint>::With();
}
