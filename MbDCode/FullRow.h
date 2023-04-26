#pragma once
#include "Vector.h"
namespace MbD {
	template <typename T>
	class FullRow : public Vector<T>
	{
	public:
		FullRow() {}
		FullRow(int i) : Vector<T>(i) {}
		FullRow(std::initializer_list<T> list) : Vector<T>{ list } {}
	};
}

