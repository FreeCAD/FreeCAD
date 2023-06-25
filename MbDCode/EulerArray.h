#pragma once
#include "FullColumn.h"

namespace MbD {

    template<typename T>
    class EulerArray : public FullColumn<T>
    {
        //
    public:
        EulerArray(int count) : FullColumn<T>(count) {}
        EulerArray(int count, const T& value) : FullColumn<T>(count, value) {}
        EulerArray(std::initializer_list<T> list) : FullColumn<T>{ list } {}
        void initialize() override;
        void equalFullColumn(std::shared_ptr<FullColumn<T>> fullCol);
        void equalFullColumnAt(std::shared_ptr<FullColumn<T>> fullCol, int i);
        virtual void calc() = 0;

    };
    template<typename T>
    inline void EulerArray<T>::initialize()
    {
    }
    template<typename T>
    inline void EulerArray<T>::equalFullColumn(std::shared_ptr<FullColumn<T>> fullCol)
    {
        this->equalArrayAt(fullCol, 0);
    }
    template<typename T>
    inline void EulerArray<T>::equalFullColumnAt(std::shared_ptr<FullColumn<T>> fullCol, int i)
    {
        this->equalArrayAt(fullCol, i);
    }
}

