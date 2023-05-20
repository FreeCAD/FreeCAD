#pragma once
#include <string>
#include <vector>

#include "FullColumn.h"

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
		virtual void removeRedundantConstraints(std::shared_ptr<std::vector<int>> redunEqnNos);
		virtual void constraintsReport();
		virtual void setqsu(std::shared_ptr<FullColumn<double>> qsuOld);
		void setName(std::string& str);
		const std::string& getName() const;

	private:
		std::string name;
	};
}

