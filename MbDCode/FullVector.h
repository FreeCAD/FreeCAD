#pragma once
#include "Array.h"

namespace MbD {
    template <typename T>
    class FullVector : public Array<T>
    {
    public:
        FullVector() {}
        FullVector(size_t count) : Array<T>(count) {}
        FullVector(size_t count, const T& value) : Array<T>(count, value) {}
        FullVector(std::initializer_list<T> list) : Array<T>{ list } {}
        double dot(std::shared_ptr<FullVector<T>> vec);
        void atiplusNumber(size_t i, T value);
        double sumOfSquares() override;
        size_t numberOfElements() override;
        void zeroSelf() override;
        void atitimes(size_t i, double factor);
    };
    template<typename T>
    inline double FullVector<T>::dot(std::shared_ptr<FullVector<T>> vec)
    {
        size_t n = this->size();
        double answer = 0.0;
        for (size_t i = 0; i < n; i++) {
            answer += this->at(i) * vec->at(i);
        }
        return answer;
    }
    template<typename T>
    inline void FullVector<T>::atiplusNumber(size_t i, T value)
    {
        this->at(i) += value;
    }
    template<>
    inline double FullVector<double>::sumOfSquares()
    {
        double sum = 0.0;
        for (size_t i = 0; i < this->size(); i++)
        {
            double element = this->at(i);
            sum += element * element;
        }
        return sum;
    }
    template<typename T>
    inline double FullVector<T>::sumOfSquares()
    {
        assert(false);
        return 0.0;
    }
    template<typename T>
    inline size_t FullVector<T>::numberOfElements()
    {
        return this->size();
    }
    template<>
    inline void FullVector<double>::zeroSelf()
    {
        for (size_t i = 0; i < this->size(); i++) {
            this->at(i) = 0.0;;
        }
    }
    template<typename T>
    inline void FullVector<T>::zeroSelf()
    {
        assert(false);
    }
    template<typename T>
    inline void FullVector<T>::atitimes(size_t i, double factor)
    {
        this->at(i) *= factor;
    }
}
