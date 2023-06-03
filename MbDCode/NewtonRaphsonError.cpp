#include "NewtonRaphsonError.h"

using namespace MbD;

//MbD::NewtonRaphsonError::NewtonRaphsonError()
//{
//}

MbD::NewtonRaphsonError::NewtonRaphsonError(const std::string& msg) : std::runtime_error(msg)
{
}
