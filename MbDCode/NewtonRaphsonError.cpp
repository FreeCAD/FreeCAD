#include "NewtonRaphsonError.h"

using namespace MbD;

//NewtonRaphsonError::NewtonRaphsonError()
//{
//}

NewtonRaphsonError::NewtonRaphsonError(const std::string& msg) : std::runtime_error(msg)
{
}
