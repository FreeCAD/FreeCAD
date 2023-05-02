#pragma once
#include <string>
#include <sstream> 

#include "Vector.h"
#include "FullMatrix.h"

namespace MbD {
	template <typename T>
	class FullColumn : public Vector<T>
	{
	public:
		FullColumn(int i) : Vector<T>(i) {}
		FullColumn(std::initializer_list<T> list) : Vector<T>{ list } {}
		std::string toString()
		{
			std::stringstream ss;

			ss << "FullColumn { ";
			for (int i = 0; i < this->size() - 1; i++) {
				ss << this->at(i) << ", ";
			}
			ss << this->back() << " }";
			return ss.str();
		}
	};

	typedef std::shared_ptr<FullColumn<double>> FullColDptr;
	//typedef std::shared_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>> FullColFMptr;
}

