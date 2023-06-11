#include "NotKinematicError.h"

using namespace MbD;

MbD::NotKinematicError::NotKinematicError(const std::string& msg) : std::runtime_error(msg)
{
}
