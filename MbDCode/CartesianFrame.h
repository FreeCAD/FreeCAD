#pragma once
#include "Item.h"

namespace MbD {
	class CartesianFrame : public Item
	{
	public:
		CartesianFrame();
		CartesianFrame(const char* str);
		void initialize();
	};
}

