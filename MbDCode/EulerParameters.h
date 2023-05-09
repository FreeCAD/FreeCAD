#pragma once
#include "EulerArray.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {
	class EulerParameters : public EulerArray
	{
		//aA aB aC pApE
	public:
		static std::unique_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> ppApEpEtimesColumn(FColDsptr col);
		static std::unique_ptr<FullMatrix<std::shared_ptr<FullMatrix<double>>>> ppApEpEtimesMatrix(FMatDsptr col);
		
	};
}

