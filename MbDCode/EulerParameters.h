#pragma once
#include "EulerArray.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {
	class EulerParameters : public EulerArray
	{
		//aA aB aC pApE
	public:
		static std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> ppApEpEtimesColumn(FullColDptr col);
		static std::shared_ptr<FullMatrix<std::shared_ptr<FullMatrix<double>>>> ppApEpEtimesMatrix(FullMatDptr col);
		
	};

	//typedef std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> FMatFColDptr;
}

