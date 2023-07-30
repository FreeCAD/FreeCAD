#include "Ln.h"

using namespace MbD;

MbD::Ln::Ln(Symsptr arg) : FunctionX(arg)
{
}

double MbD::Ln::getValue()
{
    return std::log(xx->getValue());
}
