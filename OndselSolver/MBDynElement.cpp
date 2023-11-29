#include "MBDynElement.h"

using namespace MbD;

void MbD::MBDynElement::initialize()
{
}

void MBDynElement::parseMBDyn(std::vector<std::string> &lines) {
    MBDynItem::parseMBDyn(lines);
}
