#include "ASMTUniversalJoint.h"
#include "UniversalJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTUniversalJoint::mbdClassNew()
{
    return CREATE<UniversalJoint>::With();
}
