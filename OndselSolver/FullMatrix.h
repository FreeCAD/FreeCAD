/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "corecrt_math_defines.h"
#include <memory>

#include "FullMatrix.ref.h"
#include "FullColumn.ref.h"
#include "FullRow.ref.h"
#include "DiagonalMatrix.ref.h"
#include "EulerParameters.ref.h"
#include "RowTypeMatrix.h"
#include "FullRow.h" // exception to normal include pattern

namespace MbD {
    template<typename T>
    class FullMatrix;

    template<typename T>
    using FMatsptr = std::shared_ptr<FullMatrix<T>>;

    template<typename T>
	class FullMatrix : public RowTypeMatrix<FRowsptr<T>>
	{
	public:
		FullMatrix() {};
		explicit FullMatrix(int m) : RowTypeMatrix<FRowsptr<T>>(m)
		{
		}
		FullMatrix(int m, int n) {
			for (int i = 0; i < m; i++) {
				auto row = std::make_shared<FullRow<T>>(n);
				this->push_back(row);
			}
		}
		FullMatrix(std::initializer_list<FRowsptr<T>> listOfRows) {
			for (auto& row : listOfRows)
			{
				this->push_back(row);
			}
		}
		FullMatrix(std::initializer_list<std::initializer_list<T>> list2D) {
			for (auto& rowList : list2D)
			{
				auto row = std::make_shared<FullRow<T>>(rowList);
				this->push_back(row);
			}
		}
		static FMatsptr<T> rotatex(T angle);
		static FMatsptr<T> rotatey(T angle);
		static FMatsptr<T> rotatez(T angle);
		static FMatsptr<T> rotatexrotDot(T angle, T angledot);
		static FMatsptr<T> rotateyrotDot(T angle, T angledot);
		static FMatsptr<T> rotatezrotDot(T angle, T angledot);
		static FMatsptr<T> rotatexrotDotrotDDot(T angle, T angleDot, T angleDDot);
		static FMatsptr<T> rotateyrotDotrotDDot(T angle, T angleDot, T angleDDot);
		static FMatsptr<T> rotatezrotDotrotDDot(T angle, T angleDot, T angleDDot);
		static FMatsptr<T> identitysptr(int n);
		static FMatsptr<T> tildeMatrix(FColDsptr col);

        virtual void identity();
		FColsptr<T> column(int j);
		FColsptr<T> timesFullColumn(FColsptr<T> fullCol);
		FColsptr<T> timesFullColumn(FullColumn<T>* fullCol);
		FMatsptr<T> timesFullMatrix(FMatsptr<T> fullMat);

        virtual FMatsptr<T> timesTransposeFullMatrix(FMatsptr<T> fullMat);

        FMatsptr<T> times(T a);
		FMatsptr<T> transposeTimesFullMatrix(FMatsptr<T> fullMat);
		FMatsptr<T> plusFullMatrix(FMatsptr<T> fullMat);
		FMatsptr<T> minusFullMatrix(FMatsptr<T> fullMat);
		FMatsptr<T> transpose();
		FMatsptr<T> negated();
		void symLowerWithUpper();
		void atiput(int i, FRowsptr<T> fullRow) override;
		void atijput(int i, int j, T value);
		void atijputFullColumn(int i, int j, FColsptr<T> fullCol);
		void atijplusFullRow(int i, int j, FRowsptr<T> fullRow);
		void atijplusNumber(int i, int j, T value);
		void atijminusNumber(int i, int j, T value);
		double sumOfSquares() override;
		void zeroSelf() override;
		FMatsptr<T> copy();
		FullMatrix<T> operator+(const FullMatrix<T> fullMat);
		FColsptr<T> transposeTimesFullColumn(const FColsptr<T> fullCol);
		void magnifySelf(T factor);
		std::shared_ptr<EulerParameters<T>> asEulerParameters();
		T trace();
		double maxMagnitude() override;
		FColsptr<T> bryantAngles();
		bool isDiagonal();
		bool isDiagonalToWithin(double ratio);
		std::shared_ptr<DiagonalMatrix<T>> asDiagonalMatrix();
		void conditionSelfWithTol(double tol);

		std::ostream& printOn(std::ostream& s) const override;
	};

    //
    // FULL MATRIX DOUBLE instantiation
    //
    class FullMatrixDouble : public FullMatrix<double> {
    public:
        FullMatrixDouble() : FullMatrix<double>() {};
        explicit FullMatrixDouble(int m) : FullMatrix<double>(m) {};
        FullMatrixDouble(int m, int n) : FullMatrix<double>(m, n) {};
        FullMatrixDouble(std::initializer_list<std::initializer_list<double>> list2D) : FullMatrix<double>(list2D) {}
        FullMatrixDouble(std::initializer_list<FRowsptr<double>> listOfRows) : FullMatrix<double>(listOfRows) {};

        std::shared_ptr<FullMatrixDouble> times(double a);
        std::shared_ptr<FullMatrixDouble> timesTransposeFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat);
        double sumOfSquares() override;
        void identity() override;
        static std::shared_ptr<MbD::FullMatrixDouble> identitysptr(int n);
    };
    using FMatDsptr = std::shared_ptr<MbD::FullMatrixDouble>;
    std::shared_ptr<FullMatrixDouble> toFMDsptr(FMatsptr<double> s) {
        return std::static_pointer_cast<FullMatrixDouble>(s);
    }

    //
    // FULL MATRIX FULL MATRIX DOUBLE instantiation
    //
    class FullMatrixFullMatrixDouble : public FullMatrix<FMatDsptr> {
    public:
        FullMatrixFullMatrixDouble() : FullMatrix<FMatDsptr>() {};
        explicit FullMatrixFullMatrixDouble(int m) : FullMatrix<FMatDsptr>(m) {};
        FullMatrixFullMatrixDouble(int m, int n) : FullMatrix<FMatDsptr>(m, n) {};

        std::shared_ptr<FullMatrixFullMatrixDouble> times(double a);
        std::shared_ptr<FullMatrixFullMatrixDouble> timesTransposeFullMatrix(std::shared_ptr<FullMatrixFullMatrixDouble> fullMat);
        double sumOfSquares() override;
        void identity() override;
        static std::shared_ptr<MbD::FullMatrixFullMatrixDouble> identitysptr(int n);



    };
    using FMatFMatDsptr = std::shared_ptr<FullMatrixFullMatrixDouble>;
    std::shared_ptr<FullMatrixFullMatrixDouble> toFMFMDsptr(std::shared_ptr<FullMatrix<FMatDsptr>> s) {
        return std::static_pointer_cast<FullMatrixFullMatrixDouble>(s);
    }

    using FColFMatDsptr = std::shared_ptr<FullColumn<FMatDsptr>>;
    using FMatFColDsptr = std::shared_ptr<FullMatrix<FColDsptr>>;


}

