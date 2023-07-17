#pragma once

#include <stdexcept>
#include <memory>
#include <vector>
#include "enum.h"

namespace MbD {
	class DiscontinuityError : virtual public std::runtime_error
	{
	protected:
		std::shared_ptr<std::vector<DiscontinuityType>> discontinuityTypes;

	public:
		//DiscontinuityError();
		explicit
			DiscontinuityError(const std::string& msg, std::shared_ptr<std::vector<DiscontinuityType>> disconTypes) :
			std::runtime_error(msg), discontinuityTypes(disconTypes)
		{
		}
		explicit DiscontinuityError(const std::string& msg) : std::runtime_error(msg)
		{
		}

		virtual ~DiscontinuityError() noexcept {}
	};
}
