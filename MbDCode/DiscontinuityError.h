#pragma once
#include <stdexcept>
#include <memory>
#include <vector>

namespace MbD {
	class DiscontinuityError : virtual public std::runtime_error
	{

	public:
		//DiscontinuityError();
		explicit DiscontinuityError(const std::string& msg);
		virtual ~DiscontinuityError() noexcept {}
	};
}
