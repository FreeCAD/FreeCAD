#pragma once
#include <memory>

#include "Item.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {
	class System;
	class PartFrame;
	template <typename T>
	class DiagonalMatrix;

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
		void setqXdot(FColDsptr x);
		FColDsptr getqXdot();
		void setomeOpO(FColDsptr x);
		FColDsptr getomeOpO();
		void setSystem(System& sys);
		void asFixed();
		void postInput() override;
		void calcPostDynCorrectorIteration() override;

		void prePosIC() override;
		void iqX(int eqnNo);
		void iqE(int eqnNo);

		int ipX = -1; 
		int ipE = -1; 
		double m = 0.0; 
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

