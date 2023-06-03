#pragma once
#include <stdexcept>
#include <memory>
#include <vector>

namespace MbD {
	class NewtonRaphsonError : virtual public std::runtime_error
	{

	public:
		//NewtonRaphsonError();
		explicit NewtonRaphsonError(const std::string& msg);
		virtual ~NewtonRaphsonError() noexcept {}
	};
}
