#pragma once
#include "Vector.h"
namespace MbD {
	template <typename T>
	class FullRow : public Vector<T>
	{
	public:
		FullRow() {}
		FullRow(size_t count) : Vector<T>(count) {}
		FullRow(size_t count, const T& value) : Vector<T>(count, value) {}
		FullRow(std::initializer_list<T> list) : Vector<T>{ list } {}
		std::shared_ptr<FullRow<T>> times(double a);
		std::shared_ptr<FullRow<T>> negated();

	};
	typedef std::shared_ptr<FullRow<double>> FullRowDptr;

	template<typename T>
	inline std::shared_ptr<FullRow<T>> FullRow<T>::times(double a)
	{
		size_t n = this->size();
		auto answer = std::make_shared<FullRow>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i) * a;
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullRow<T>> FullRow<T>::negated()
	{
		return this->times(-1.0);
	}
}

