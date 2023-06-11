#include "TooManyTriesError.h"

MbD::TooManyTriesError::TooManyTriesError(const std::string& msg) : std::runtime_error(msg)
{
}
