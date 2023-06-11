#include "TooSmallStepSizeError.h"

MbD::TooSmallStepSizeError::TooSmallStepSizeError(const std::string& msg) : std::runtime_error(msg)
{
}
