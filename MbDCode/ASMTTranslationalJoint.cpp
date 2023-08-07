#include "ASMTTranslationalJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTTranslationalJoint::mbdClassNew()
{
    return CREATE<TranslationalJoint>::With();
}
