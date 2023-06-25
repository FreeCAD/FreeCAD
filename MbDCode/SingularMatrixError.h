#pragma once
#include <stdexcept>
#include <memory>
#include <vector>

#include "FullColumn.h"

namespace MbD {
	class SingularMatrixError : virtual public std::runtime_error
	{
	protected:
		std::shared_ptr<std::vector<int>> redundantEqnNos;

	public:
		explicit
			SingularMatrixError(const std::string& msg, std::shared_ptr<FullColumn<int>> redunEqnNos) :
			std::runtime_error(msg), redundantEqnNos(redunEqnNos)
		{
		}
		explicit SingularMatrixError(const std::string& msg) : std::runtime_error(msg)
		{
		}

		virtual ~SingularMatrixError() noexcept {}

		virtual std::shared_ptr<std::vector<int>> getRedundantEqnNos() const noexcept {
			return redundantEqnNos;
		}
	};
}

