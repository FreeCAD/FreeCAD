#pragma once
#include <map>
#include <cmath>
#include <sstream> 

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
		double rootMeanSquare();
		int numberOfElements();
		double sumOfSquares();
		void atiplusNumber(int i, double value);
		void zeroSelf();
		double maxElement();

		virtual std::ostream& printOn(std::ostream& s) const;
		friend std::ostream& operator<<(std::ostream& s, const SparseVector& spVec)
		{
			return spVec.printOn(s);
		}
	};
	template<typename T>
	inline double SparseVector<T>::rootMeanSquare()
	{
		return std::sqrt(this->sumOfSquares() / this->numberOfElements());
	}
	template<typename T>
	inline int SparseVector<T>::numberOfElements()
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
	inline void SparseVector<double>::atiplusNumber(int i, double value)
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
	template<typename T>
	inline std::ostream& SparseVector<T>::printOn(std::ostream& s) const
	{
		s << "{";
		auto index = 0;
		for (const auto& keyValue : *this) {
			if (index > 0) s << ", ";
			s << keyValue.first;
			s << "->";
			s << keyValue.second;
			index++;
		}
		s << "}";
		return s;
	}
}

