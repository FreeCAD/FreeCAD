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
#include "FullRow.h" // now that refs are resolved, go do the full systems
//#include "FullColumn.h"
//#include "DiagonalMatrix.h"
//#include "EulerParameters.h"

namespace MbD {
    //
    // FULL MATRIX DOUBLE
    //
    class FullMatrixDouble : public RowTypeMatrix<FRowsptr<double>> {
    public:
        FullMatrixDouble() = default;
        explicit FullMatrixDouble(int m) : RowTypeMatrix<FRowsptr<double>>(m)
        {
        }
        FullMatrixDouble(int m, int n) {
            for (int i = 0; i < m; i++) {
                auto row = std::make_shared<FullRow<double>>(n);
                this->push_back(row);
            }
        }
        FullMatrixDouble(std::initializer_list<FRowsptr<double>> listOfRows) {
            for (auto& row : listOfRows)
            {
                this->push_back(row);
            }
        }
        FullMatrixDouble(std::initializer_list<std::initializer_list<double>> list2D) {
            for (auto& rowList : list2D)
            {
                auto row = std::make_shared<FullRow<double>>(rowList);
                this->push_back(row);
            }
        }

        std::shared_ptr<FullMatrixDouble> times(double a);
        std::shared_ptr<FullMatrixDouble> timesTransposeFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat);
        void identity();
        static std::shared_ptr<MbD::FullMatrixDouble> identitysptr(int n);
        double sumOfSquares() override;
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
		static std::shared_ptr<FullMatrixDouble> rotatexrotDotrotDDot(double angle, double angleDot, double angleDDot);
		static std::shared_ptr<FullMatrixDouble> rotateyrotDotrotDDot(double angle, double angleDot, double angleDDot);
		static std::shared_ptr<FullMatrixDouble> rotatezrotDotrotDDot(double angle, double angleDot, double angleDDot);
        static std::shared_ptr<FullMatrixDouble> tildeMatrix(FColDsptr col);
        void zeroSelf() override;
        FColsptr<double> column(int j);

        void atiput(int i, FRowsptr<double> fullRow) override;
        void atijput(int i, int j, double value);
        std::shared_ptr<FullMatrixDouble> copy();
        double maxMagnitude() override;
        FullMatrixDouble operator+(const FullMatrixDouble fullMat);
		std::shared_ptr<FullMatrixDouble> minusFullMatrix(std::shared_ptr<FullMatrixDouble> fullMat);
        FColsptr<double> transposeTimesFullColumn(FColsptr<double> fullCol);
        void symLowerWithUpper();
        void atijputFullColumn(int i1, int j1, FColsptr<double> fullCol);
        void atijplusFullRow(int i, int j, FRowsptr<double> fullRow);
        void atijplusNumber(int i, int j, double value);
        void atijminusNumber(int i, int j, double value);
        void magnifySelf(double factor);
        std::shared_ptr<EulerParameters<double>> asEulerParameters();
        FColsptr<double> bryantAngles();
        double trace();
        bool isDiagonal();
        bool isDiagonalToWithin(double ratio);
        std::shared_ptr<DiagonalMatrix> asDiagonalMatrix();
        void conditionSelfWithTol(double tol);
        std::ostream& printOn(std::ostream& s) const override;
        FColsptr<double> timesFullColumn(FColsptr<double> fullCol);
        // FColsptr<double> timesFullColumn(FullColumn<double>* fullCol);
    };

    //
    // FULL MATRIX FULL MATRIX DOUBLE
    //
    class FullMatrixFullMatrixDouble : public RowTypeMatrix<FRowsptr<FMatDsptr>> {
    public:
        FullMatrixFullMatrixDouble() = default;
		explicit FullMatrixFullMatrixDouble(int m) : RowTypeMatrix<FRowsptr<FMatDsptr>>(m)
		{
		}
        FullMatrixFullMatrixDouble(int m, int n) {
            for (int i = 0; i < m; i++) {
                auto row = std::make_shared<FullRow<FMatDsptr>>(n);
                this->push_back(row);
            }
        }

        double maxMagnitude() override;
        void zeroSelf() override;
        std::shared_ptr<FullMatrixFullMatrixDouble> times(double a);
        // std::shared_ptr<FullMatrixFullMatrixDouble> timesTransposeFullMatrix(std::shared_ptr<FullMatrixFullMatrixDouble> fullMat);
        double sumOfSquares() override;
        void identity();
        static std::shared_ptr<MbD::FullMatrixFullMatrixDouble> identitysptr(int n);
    };
    //
    // FULL MATRIX FULL COLUMN DOUBLE
    //
    class FullMatrixFullColumnDouble : public RowTypeMatrix<FRowsptr<FColDsptr>> {
    public:
        FullMatrixFullColumnDouble() = default;
		explicit FullMatrixFullColumnDouble(int m) : RowTypeMatrix<FRowsptr<FColDsptr>>(m)
		{
		}
        FullMatrixFullColumnDouble(int m, int n) {
            for (int i = 0; i < m; i++) {
                auto row = std::make_shared<FullRow<FColDsptr>>(n);
                this->push_back(row);
            }
        }

        double maxMagnitude() override;
        void zeroSelf() override;
        double sumOfSquares() override;
        void symLowerWithUpper();
        std::shared_ptr<FullMatrixFullColumnDouble> times(double a);
    };
}
