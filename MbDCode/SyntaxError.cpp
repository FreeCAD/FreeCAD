#include "SyntaxError.h"

using namespace MbD;

SyntaxError::SyntaxError(const std::string& msg) : std::runtime_error(msg)
{
}
