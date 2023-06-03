#include "SimulationStoppingError.h"

using namespace MbD;

MbD::SimulationStoppingError::SimulationStoppingError(const std::string& msg) : std::runtime_error(msg)
{
}
