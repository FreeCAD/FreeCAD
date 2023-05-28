#pragma once
#include <map>
#include <cmath>

namespace MbD {
	template <typename T>
	class SparseVector : public std::map<int, T>
	{
	public:
		int n;
		SparseVector() {}
		SparseVector(int n) : std::map<int, T>(), n(n) {}
		SparseVector(std::initializer_list<std::pair<const int, T>> list) : std::map<int, T>{ list } {}
		SparseVector(std::initializer_list<std::initializer_list<T>> list) {
			for (auto& pair : list) {
				int i = 0;
				int index;
				T value;
				for (auto& element : pair) {
					if (i == 0) index = (int)std::round(element); ;
					if (i == 1) value = element;
					i++;
				}
				this->insert(std::pair<const int, double>(index, value));
			}
		}
	};
}

