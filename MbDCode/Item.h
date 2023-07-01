#pragma once
#include <string>
#include <vector>

#include "FullColumn.h"
#include "FullMatrix.h"
#include "DiagonalMatrix.h"
#include "SparseMatrix.h"
#include "enum.h"

namespace MbD {
	class System;
	class Constraint;
	class StateData;

	class Item
	{
		//name
	public:
		Item();
		Item(const char* str);
		virtual System* root();
		virtual void initialize();
		virtual void initializeLocally();
		virtual void initializeGlobally();
		virtual void postInput();
		virtual void calcPostDynCorrectorIteration();
		virtual void constraintsReport();
		virtual void setqsu(FColDsptr qsuOld);
		virtual void useEquationNumbers();
		virtual void logString(std::string& str);
		void logString(const char* chars);

		virtual void prePosIC();
		virtual void prePosKine();

		virtual void fillPosICError(FColDsptr col);
		virtual void fillPosICJacob(FMatDsptr mat);
		virtual void fillPosICJacob(SpMatDsptr mat);
		virtual void postPosIC();
		virtual void postPosICIteration();
		virtual void removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos);
		virtual void reactivateRedundantConstraints();
		virtual void fillPosKineError(FColDsptr col);
		virtual void fillPosKineJacob(SpMatDsptr mat);
		virtual void fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints);
		virtual void fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints);
		virtual void fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints);

		virtual void fillqsu(FColDsptr col);
		virtual void fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat);
		virtual void fillqsulam(FColDsptr col);
		virtual void setqsulam(FColDsptr col);
		virtual void outputStates();
		virtual void preDyn();
		virtual void postDyn();
		virtual std::string classname();
		virtual void preDynFirstStep();
		virtual void postDynFirstStep();
		virtual void preDynStep();
		virtual void postDynStep();
		virtual void storeDynState();
		virtual double suggestSmallerOrAcceptDynFirstStepSize(double hnew);
		virtual double suggestSmallerOrAcceptDynStepSize(double hnew);
		virtual void preVelIC();
		virtual void postVelIC();
		virtual void fillqsudot(FColDsptr col);
		virtual void fillqsudotWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat);
		virtual void fillVelICError(FColDsptr col);
		virtual void fillVelICJacob(SpMatDsptr mat);
		virtual void setqsudotlam(FColDsptr col);
		virtual void setqsudot(FColDsptr col);
		virtual void setqsuddotlam(FColDsptr col);

		virtual void preAccIC();
		virtual void postAccIC();
		virtual void postAccICIteration();
		virtual void fillqsuddotlam(FColDsptr col);
		virtual void fillAccICIterError(FColDsptr col);
		virtual void fillAccICIterJacob(SpMatDsptr mat);
		virtual std::shared_ptr<StateData> stateData();
		virtual void discontinuityAtaddTypeTo(double t, std::shared_ptr<std::vector<DiscontinuityType>> disconTypes);
		virtual double checkForDynDiscontinuityBetween(double tprev, double t);

		void setName(std::string& str);
		const std::string& getName() const;

		virtual std::ostream& printOn(std::ostream& s) const;
		friend std::ostream& operator<<(std::ostream& s, const Item& item)
		{
			if (&item) {
				return item.printOn(s);
			}
			else {
				s << "NULL";
			}
			return s;
		}

		std::string name;
		Item* owner = nullptr;	//Use raw pointer when pointing backwards.
	};
}

