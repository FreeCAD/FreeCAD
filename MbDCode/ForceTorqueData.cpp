#include "ForceTorqueData.h"

using namespace MbD;

std::ostream& ForceTorqueData::printOn(std::ostream& s) const
{
    s << "aFIO = " << *aFIO << std::endl;
    s << "aTIO = " << *aTIO << std::endl;
    return s;
}
