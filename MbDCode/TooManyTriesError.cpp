#include "TooManyTriesError.h"

using namespace MbD;

TooManyTriesError::TooManyTriesError(const std::string& msg) : std::runtime_error(msg)
{
}
