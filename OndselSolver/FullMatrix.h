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
	class FullMatrixTemplate : public RowTypeMatrix<FRowsptr<T>>
	{
	public:
		FullMatrixTemplate() = default;
		explicit FullMatrixTemplate(int m) : RowTypeMatrix<FRowsptr<T>>(m)
		{
		}
		FullMatrixTemplate(int m, int n) {
			for (int i = 0; i < m; i++) {
				auto row = std::make_shared<FullRow<T>>(n);
				this->push_back(row);
			}
		}
		FullMatrixTemplate(std::initializer_list<FRowsptr<T>> listOfRows) {
			for (auto& row : listOfRows)
			{
				this->push_back(row);
			}
		}
		FullMatrixTemplate(std::initializer_list<std::initializer_list<T>> list2D) {
			for (auto& rowList : list2D)
			{
				auto row = std::make_shared<FullRow<T>>(rowList);
				this->push_back(row);
			}
		}
//		static std::shared_ptr<FullMatrixTemplate<T>> rotatex(T angle);
//		static std::shared_ptr<FullMatrixTemplate<T>> rotatey(T angle);
//		static std::shared_ptr<FullMatrixTemplate<T>> rotatez(T angle);
//		static std::shared_ptr<FullMatrixTemplate<T>> rotatexrotDot(T angle, T angledot);
//		static std::shared_ptr<FullMatrixTemplate<T>> rotateyrotDot(T angle, T angledot);
//		static std::shared_ptr<FullMatrixTemplate<T>> rotatezrotDot(T angle, T angledot);
		static std::shared_ptr<FullMatrixTemplate<T>> rotatexrotDotrotDDot(T angle, T angleDot, T angleDDot);
		static std::shared_ptr<FullMatrixTemplate<T>> rotateyrotDotrotDDot(T angle, T angleDot, T angleDDot);
		static std::shared_ptr<FullMatrixTemplate<T>> rotatezrotDotrotDDot(T angle, T angleDot, T angleDDot);
		static std::shared_ptr<FullMatrixTemplate<T>> identitysptr(int n);
		static std::shared_ptr<FullMatrixTemplate<T>> tildeMatrix(FColDsptr col);

        virtual void identity();
		FColsptr<T> column(int j);
		FColsptr<T> timesFullColumn(FColsptr<T> fullCol);
		FColsptr<T> timesFullColumn(FullColumn<T>* fullCol);
		// std::shared_ptr<FullMatrixTemplate<T>> timesFullMatrix(std::shared_ptr<FullMatrixTemplate<T>> fullMat);

        virtual std::shared_ptr<FullMatrixTemplate<T>> timesTransposeFullMatrix(std::shared_ptr<FullMatrixTemplate<T>> fullMat);

        std::shared_ptr<FullMatrixTemplate<T>> times(T a);
		// std::shared_ptr<FullMatrixTemplate<T>> transposeTimesFullMatrix(std::shared_ptr<FullMatrixTemplate<T>> fullMat);
		// std::shared_ptr<FullMatrixTemplate<T>> plusFullMatrix(std::shared_ptr<FullMatrixTemplate<T>> fullMat);
		std::shared_ptr<FullMatrixTemplate<T>> minusFullMatrix(std::shared_ptr<FullMatrixTemplate<T>> fullMat);
		// std::shared_ptr<FullMatrixTemplate<T>> transpose();
//		std::shared_ptr<FullMatrixTemplate<T>> negated();
		void symLowerWithUpper();
		void atiput(int i, FRowsptr<T> fullRow) override;
		void atijput(int i, int j, T value);
		void atijputFullColumn(int i, int j, FColsptr<T> fullCol);
		void atijplusFullRow(int i, int j, FRowsptr<T> fullRow);
		void atijplusNumber(int i, int j, T value);
		void atijminusNumber(int i, int j, T value);
		double sumOfSquares() override;
		void zeroSelf() override;
//		std::shared_ptr<FullMatrixTemplate<T>> copy();
		FullMatrixTemplate<T> operator+(const FullMatrixTemplate<T> fullMat);
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
    class FullMatrixDouble : public FullMatrixTemplate<double> {
    public:
        FullMatrixDouble() : FullMatrixTemplate<double>() {};
        explicit FullMatrixDouble(int m) : FullMatrixTemplate<double>(m) {};
        FullMatrixDouble(int m, int n) : FullMatrixTemplate<double>(m, n) {};
        FullMatrixDouble(std::initializer_list<std::initializer_list<double>> list2D) : FullMatrixTemplate<double>(list2D) {}
        FullMatrixDouble(std::initializer_list<FRowsptr<double>> listOfRows) : FullMatrixTemplate<double>(listOfRows) {};

        std::shared_ptr<FullMatrixDouble> times(double a);
        std::shared_ptr<FullMatrixDouble> timesTransposeFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat);
        void identity() override;
        static std::shared_ptr<MbD::FullMatrixDouble> identitysptr(int n);
        double sumOfSquares();
        std::shared_ptr<FullMatrixDouble> transposeTimesFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat);
        std::shared_ptr<FullMatrixDouble> timesFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat);
        std::shared_ptr<FullMatrixDouble> transpose();
		std::shared_ptr<FullMatrixDouble> negated();
        std::shared_ptr<FullMatrixDouble> plusFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat);
		static std::shared_ptr<FullMatrixDouble> rotatex(double angle);
		static std::shared_ptr<FullMatrixDouble> rotatey(double angle);
		static std::shared_ptr<FullMatrixDouble> rotatez(double angle);
		static std::shared_ptr<FullMatrixDouble> rotatexrotDot(double angle, double angledot);
		static std::shared_ptr<FullMatrixDouble> rotateyrotDot(double angle, double angledot);
		static std::shared_ptr<FullMatrixDouble> rotatezrotDot(double angle, double angledot);
		std::shared_ptr<FullMatrixDouble> copy();
    };

    //
    // FULL MATRIX FULL MATRIX DOUBLE instantiation
    //
    class FullMatrixFullMatrixDouble : public FullMatrixTemplate<FMatDsptr> {
    public:
        FullMatrixFullMatrixDouble() : FullMatrixTemplate<FMatDsptr>() {};
        explicit FullMatrixFullMatrixDouble(int m) : FullMatrixTemplate<FMatDsptr>(m) {};
        FullMatrixFullMatrixDouble(int m, int n) : FullMatrixTemplate<FMatDsptr>(m, n) {};

        std::shared_ptr<FullMatrixFullMatrixDouble> times(double a);
        std::shared_ptr<FullMatrixFullMatrixDouble> timesTransposeFullMatrix(std::shared_ptr<FullMatrixFullMatrixDouble> fullMat);
        double sumOfSquares() override;
        void identity() override;
        static std::shared_ptr<MbD::FullMatrixFullMatrixDouble> identitysptr(int n);
    };
    //
    // FULL MATRIX FULL COLUMN DOUBLE instantiation
    //
    class FullMatrixFullColumnDouble : public FullMatrixTemplate<FColDsptr> {
    public:
        FullMatrixFullColumnDouble() : FullMatrixTemplate<FColDsptr>() {};
        explicit FullMatrixFullColumnDouble(int m) : FullMatrixTemplate<FColDsptr>(m) {};
        FullMatrixFullColumnDouble(int m, int n) : FullMatrixTemplate<FColDsptr>(m, n) {};

        std::shared_ptr<FullMatrixFullColumnDouble> times(double a);
        std::shared_ptr<FullMatrixFullColumnDouble> timesTransposeFullMatrix(std::shared_ptr<FullMatrixFullColumnDouble> fullMat);
        double sumOfSquares() override;
        void identity() override;
        static std::shared_ptr<MbD::FullMatrixFullColumnDouble> identitysptr(int n);
    };
}
