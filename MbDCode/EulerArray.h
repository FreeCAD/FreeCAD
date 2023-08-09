/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
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
        void equalFullColumn(FColsptr<T> fullCol);
        void equalFullColumnAt(FColsptr<T> fullCol, int i);
        virtual void calc() = 0;

    };
    template<typename T>
    inline void EulerArray<T>::initialize()
    {
    }
    template<typename T>
    inline void EulerArray<T>::equalFullColumn(FColsptr<T> fullCol)
    {
        this->equalArrayAt(fullCol, 0);
    }
    template<typename T>
    inline void EulerArray<T>::equalFullColumnAt(FColsptr<T> fullCol, int i)
    {
        this->equalArrayAt(fullCol, i);
    }
}

