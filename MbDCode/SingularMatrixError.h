#pragma once
#include <stdexcept>
#include <memory>
#include <vector>

namespace MbD {
	class SingularMatrixError : virtual public std::runtime_error
	{
	protected:

		std::shared_ptr<std::vector<int>> redundantEqnNos;

	public:
		explicit
			SingularMatrixError(const std::string& msg, std::shared_ptr<std::vector<int>> redunEqnNos) :
			std::runtime_error(msg), redundantEqnNos(redunEqnNos)
		{
		}

		virtual ~SingularMatrixError() noexcept {}

		virtual std::shared_ptr<std::vector<int>> getRedundantEqnNos() const noexcept {
			return redundantEqnNos;
		}
	};
}

