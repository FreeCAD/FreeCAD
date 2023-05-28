#pragma once
#include <string>
#include <vector>

#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {

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
		virtual void setqsu(std::shared_ptr<FullColumn<double>> qsuOld);
		virtual void useEquationNumbers();

		virtual void prePosIC();
		virtual void fillPosICError(FColDsptr col);
		virtual void fillPosICJacob(FMatDsptr mat);
		virtual void prePostIC();
		virtual void prePostICIteration();
		virtual void prePostICRestart();
		virtual void removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos);
		virtual void fillPosKineError(FColDsptr col);
		virtual void fillPosKineJacob(FMatDsptr mat);

		void setName(std::string& str);
		const std::string& getName() const;

	private:
		std::string name;
	};
}

