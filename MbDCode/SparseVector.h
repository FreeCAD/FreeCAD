#pragma once
#include <map>
#include <cmath>

namespace MbD {
	template <typename T>
	class SparseVector : public std::map<size_t, T>
	{
	public:
		size_t n;
		SparseVector() {}
		SparseVector(size_t n) : std::map<size_t, T>(), n(n) {}
		SparseVector(std::initializer_list<std::pair<const size_t, T>> list) : std::map<size_t, T>{ list } {}
		SparseVector(std::initializer_list<std::initializer_list<T>> list) {
			for (auto& pair : list) {
				size_t i = 0;
				size_t index;
				T value;
				for (auto& element : pair) {
					if (i == 0) index = (size_t)std::round(element); ;
					if (i == 1) value = element;
					i++;
				}
				this->insert(std::pair<const size_t, double>(index, value));
			}
		}
		void atiminusNumber(size_t i, double value);
		double rootMeanSquare();
		size_t numberOfElements();
		double sumOfSquares();
		void atiplusNumber(size_t i, double value);
		void zeroSelf();
		double maxElement();
	};
	template<>
	inline void SparseVector<double>::atiminusNumber(size_t i, double value)
	{
		//auto val = this->at(i);
		auto val = (*this)[i];
		this->at(i) = val - value;
	}
	template<typename T>
	inline double SparseVector<T>::rootMeanSquare()
	{
		return std::sqrt(this->sumOfSquares() / this->numberOfElements());
	}
	template<typename T>
	inline size_t SparseVector<T>::numberOfElements()
	{
		return n;
	}
	template<typename T>
	inline double SparseVector<T>::sumOfSquares()
	{
		double sum = 0.0;
		for (auto const& keyValue : *this)
		{
			sum += keyValue.second * keyValue.second;
		}
		return sum;
	}
	template<>
	inline void SparseVector<double>::atiplusNumber(size_t i, double value)
	{
		this->at(i) += value;
	}
	template<>
	inline void SparseVector<double>::zeroSelf()
	{
		this->clear();
	}
	template<>
	inline double SparseVector<double>::maxElement()
	{
		double max = 0.0;
		for (const auto& keyValue : *this) {
			auto val = keyValue.second;
			if (val < 0.0) val = -val;
			if (max < val) max = val;
		}
		return max;
	}
}

