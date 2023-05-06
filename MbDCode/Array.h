#pragma once
#include <vector>
#include <memory>

namespace MbD {
	template <typename T>
	class Array : public std::vector<T>
	{
	public:
		Array(){}
        Array(size_t count) : std::vector<T>(count) {}
        Array(size_t count, const T& value) : std::vector<T>(count, value) {}
        Array(std::initializer_list<T> list) : std::vector<T>{ list } {}
        void copy(std::shared_ptr<Array<T>> x);
    };
    template<typename T>
    inline void Array<T>::copy(std::shared_ptr<Array<T>> x)
    {
        for (int i = 0; i < x->size(); i++) {
            this->at(i) = x->at(i);
        }
    }
}

