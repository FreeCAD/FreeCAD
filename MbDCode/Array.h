#pragma once
#include <vector>

namespace MbD {
	template <typename T>
	class Array : public std::vector<T>
	{
	public:
		Array(){}
		Array(int i) : std::vector<T>(i) {}
		Array(std::initializer_list<T> list) : std::vector<T>{ list } {}
	};
}

