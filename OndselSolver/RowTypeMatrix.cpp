/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "RowTypeMatrix.h"

using namespace MbD;

template<typename T>
int RowTypeMatrix<T>::nrow() {
    return (int) this->size();
}

template<typename T>
int RowTypeMatrix<T>::ncol() {
    return this->at(0)->numberOfElements();
}
