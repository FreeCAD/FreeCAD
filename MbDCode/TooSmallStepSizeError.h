#pragma once

#include <stdexcept>
#include <memory>
#include <vector>

namespace MbD {
	class TooSmallStepSizeError : virtual public std::runtime_error
	{

	public:
		//TooSmallStepSizeError();
		explicit TooSmallStepSizeError(const std::string& msg);
		virtual ~TooSmallStepSizeError() noexcept {}
	};
}
