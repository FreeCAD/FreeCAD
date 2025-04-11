/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <stdexcept>
#include <memory>
#include <vector>

#include "FullColumn.h"

namespace MbD {
	class SingularMatrixError : virtual public std::runtime_error
	{
	protected:
		std::shared_ptr<std::vector<size_t>> redundantEqnNos;

	public:
		explicit
			SingularMatrixError(const std::string& msg, std::shared_ptr<FullColumn<size_t>> redunEqnNos) :
			std::runtime_error(msg), redundantEqnNos(redunEqnNos)
		{
		}
		explicit SingularMatrixError(const std::string& msg) : std::runtime_error(msg)
		{
		}

		virtual ~SingularMatrixError() noexcept {}

		virtual std::shared_ptr<std::vector<size_t>> getRedundantEqnNos() const noexcept {
			return redundantEqnNos;
		}
	};
}

