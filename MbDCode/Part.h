#pragma once
#include <memory>

#include "Item.h"
#include "PartFrame.h"
#include "FullColumn.h"


namespace MbD {
	class PartFrame;

	class Part : public Item
	{
	public:
		Part() {
			partFrame = std::make_shared<PartFrame>();
		}
		void setqX(FullColumn<double>* x) {
			partFrame.get()->setqX(x);
		}
		FullColumn<double>* getqX() {
			return partFrame.get()->getqX();
		}
		//ToDo: Needed members ipX ipE m aJ partFrame pX pXdot pE pEdot mX mE mEdot pTpE ppTpEpE ppTpEpEdot 
		std::shared_ptr<PartFrame> partFrame;
	};
}

