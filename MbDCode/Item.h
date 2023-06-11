#pragma once
#include <string>
#include <vector>

#include "FullColumn.h"
#include "FullMatrix.h"
#include "DiagonalMatrix.h"
#include "SparseMatrix.h"

namespace MbD {

	class Constraint;

	class Item
	{
		//name
	public:
		Item();
		Item(const char* str);
		virtual void initialize();
		virtual void initializeLocally();
		virtual void initializeGlobally();
		virtual void postInput();
		virtual void calcPostDynCorrectorIteration();
		virtual void constraintsReport();
		virtual void setqsu(FColDsptr qsuOld);
		virtual void useEquationNumbers();
		virtual void logString(std::string& str);

		virtual void prePosIC();
		virtual void fillPosICError(FColDsptr col);
		virtual void fillPosICJacob(FMatDsptr mat);
		virtual void fillPosICJacob(SpMatDsptr mat);
		virtual void prePostIC();
		virtual void prePostICIteration();
		virtual void prePostICRestart();
		virtual void postPosIC();
		virtual void postPosICIteration();
		virtual void removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos);
		virtual void reactivateRedundantConstraints();
		virtual void fillPosKineError(FColDsptr col);
		virtual void fillPosKineJacob(FMatDsptr mat);
		virtual void fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints);
		virtual void fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints);
		
		virtual void fillqsu(FColDsptr col);
		virtual void fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat);
		virtual void fillqsulam(FColDsptr col);
		virtual void setqsulam(FColDsptr col);
		virtual void outputStates();
		virtual void preDyn();
		virtual std::string classname();
		virtual void preDynFirstStep();
		virtual void preDynStep();
		virtual double suggestSmallerOrAcceptDynFirstStepSize(double hnew);
		

		void setName(std::string& str);
		const std::string& getName() const;

		std::string name;
	};
}

