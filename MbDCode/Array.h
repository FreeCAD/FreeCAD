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
        void copy(Array<T>* x);
    };
    template<typename T>
    inline void Array<T>::copy(Array<T>* x)
    {
        for (int i = 0; i < x->size(); i++) {
            this->at(i) = x->at(i);
        }
    }
}

