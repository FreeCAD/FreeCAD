#pragma once
#include <memory>

#include "Item.h"
#include "System.h"
#include "PartFrame.h"
#include "FullColumn.h"
#include "DiagonalMatrix.h"

namespace MbD {
	class System;
	class PartFrame;

	class Part : public Item
	{
		//ToDo: ipX ipE m aJ partFrame pX pXdot pE pEdot mX mE mEdot pTpE ppTpEpE ppTpEpEdot 
	public:
		Part();
		Part(const char* str);
		void initialize();
		void initializeLocally() override;
		void initializeGlobally() override;
		void setqX(FColDsptr x);
		FColDsptr getqX();
		void setqE(FColDsptr x);
		FColDsptr getqE();
		void setSystem(System& sys);
		void asFixed();

		int ipX; 
		int ipE; 
		double m; 
		std::shared_ptr<DiagonalMatrix<double>> aJ;
		std::shared_ptr<PartFrame> partFrame;
		FColDsptr pX;
		FColDsptr pXdot;
		FColDsptr pE;
		FColDsptr pEdot;
		std::shared_ptr<DiagonalMatrix<double>> mX;
		std::shared_ptr<DiagonalMatrix<double>> mE;
		FMatDsptr mEdot;
		FColDsptr pTpE;
		FMatDsptr ppTpEpE;
		FMatDsptr ppTpEpEdot;
	};
}

