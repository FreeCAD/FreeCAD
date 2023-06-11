#include "DiscontinuityError.h"

using namespace MbD;

MbD::DiscontinuityError::DiscontinuityError(const std::string& msg) : std::runtime_error(msg)
{
}
