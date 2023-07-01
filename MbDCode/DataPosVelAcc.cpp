#include <ostream>

#include "DataPosVelAcc.h"

using namespace MbD;

std::ostream& DataPosVelAcc::printOn(std::ostream& s) const
{
    s << "refData = " << *refData << std::endl;
    s << "rFfF = " << *rFfF << std::endl;
    s << "vFfF = " << *vFfF << std::endl;
    s << "omeFfF = " << *omeFfF << std::endl;
    s << "aFfF = " << *aFfF << std::endl;
    s << "alpFfF = " << *alpFfF << std::endl;
    s << "aAFf = " << *aAFf;
    return s;
}
