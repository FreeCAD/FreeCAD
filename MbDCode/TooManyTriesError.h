#pragma once

#include <stdexcept>
#include <memory>
#include <vector>

namespace MbD {
	class TooManyTriesError : virtual public std::runtime_error
	{

	public:
		//TooManyTriesError();
		explicit TooManyTriesError(const std::string& msg);
		virtual ~TooManyTriesError() noexcept {}
	};
}
