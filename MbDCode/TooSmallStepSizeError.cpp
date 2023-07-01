#include "TooSmallStepSizeError.h"

using namespace MbD;

TooSmallStepSizeError::TooSmallStepSizeError(const std::string& msg) : std::runtime_error(msg)
{
}
