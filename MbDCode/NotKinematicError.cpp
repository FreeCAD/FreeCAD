#include "NotKinematicError.h"

using namespace MbD;

NotKinematicError::NotKinematicError(const std::string& msg) : std::runtime_error(msg)
{
}
