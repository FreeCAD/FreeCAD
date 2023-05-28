#pragma once
#include "Array.h"

namespace MbD {
    template <typename T>
    class Vector : public Array<T>
    {
    public:
        Vector() {}
        Vector(size_t count) : Array<T>(count) {}
        Vector(size_t count, const T& value) : Array<T>(count, value) {}
        Vector(std::initializer_list<T> list) : Array<T>{ list } {}
        double dot(std::shared_ptr<Vector<T>> vec);
    };
    template<typename T>
    inline double Vector<T>::dot(std::shared_ptr<Vector<T>> vec)
    {
        size_t n = this->size();
        double answer = 0.0;
        for (size_t i = 0; i < n; i++) {
            answer += this->at(i) * vec->at(i);
        }
        return answer;
    }
}
