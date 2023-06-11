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
		void fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints) override;
		void fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints) override;
		void fillqsu(FColDsptr col) override;
		void fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat) override;
		void fillqsulam(FColDsptr col) override;
		void useEquationNumbers() override;
		void setqsu(FColDsptr col) override;
		void setqsulam(FColDsptr col) override;
		void postPosICIteration() override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos) override;
		void reactivateRedundantConstraints() override;
		void constraintsReport() override;
		void postPosIC() override;
		void outputStates() override;
		void preDyn() override;

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

