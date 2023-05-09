#include <memory>

#include "EulerParameters.h"
#include "FullColumn.h"
#include "FullMatrix.h"

using namespace MbD;

std::unique_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> EulerParameters::ppApEpEtimesColumn(FColDsptr col)
{
	double a2c1 = 2 * col->at(0);
	double a2c2 = 2 * col->at(1);
	double a2c3 = 2 * col->at(2);
	double m2c1 = 0 - a2c1;
	double m2c2 = 0 - a2c2;
	double m2c3 = 0 - a2c3;
	auto col11 = std::make_shared<FullColumn<double>>(ListD{ a2c1, m2c2, m2c3 });
	auto col12 = std::make_shared<FullColumn<double>>(ListD{ a2c2, a2c1, 0 });
	auto col13 = std::make_shared<FullColumn<double>>(ListD{ a2c3, 0, a2c1 });
	auto col14 = std::make_shared<FullColumn<double>>(ListD{ 0, m2c3, a2c2 });
	auto col22 = std::make_shared<FullColumn<double>>(ListD{ m2c1, a2c2, m2c3 });
	auto col23 = std::make_shared<FullColumn<double>>(ListD{ 0, a2c3, a2c2 });
	auto col24 = std::make_shared<FullColumn<double>>(ListD{ a2c3, 0, m2c1 });
	auto col33 = std::make_shared<FullColumn<double>>(ListD{ m2c1, m2c2, a2c3 });
	auto col34 = std::make_shared<FullColumn<double>>(ListD{ m2c2, a2c1, 0 });
	auto col44 = std::make_shared<FullColumn<double>>(ListD{ a2c1, a2c2, a2c3 });
	auto answer = std::make_unique<FullMatrix<std::shared_ptr<FullColumn<double>>>>(4, 4);
	auto row1 = answer->at(0);
	row1->at(0) = col11;
	row1->at(1) = col12;
	row1->at(2) = col13;
	row1->at(3) = col14;
	auto row2 = answer->at(1);
	row2->at(0) = col12;
	row2->at(1) = col22;
	row2->at(2) = col23;
	row2->at(3) = col24;
	auto row3 = answer->at(2);
	row3->at(0) = col13;
	row3->at(1) = col23;
	row3->at(2) = col33;
	row3->at(3) = col34;
	auto row4 = answer->at(3);
	row4->at(0) = col14;
	row4->at(1) = col24;
	row4->at(2) = col34;
	row4->at(3) = col44;
	return answer;
}

std::unique_ptr<FullMatrix<std::shared_ptr<FullMatrix<double>>>> EulerParameters::ppApEpEtimesMatrix(FMatDsptr mat)
{
	FRowDsptr a2m1 = mat->at(0)->times(2.0);
	FRowDsptr a2m2 = mat->at(1)->times(2.0);
	FRowDsptr a2m3 = mat->at(2)->times(2.0);
	FRowDsptr m2m1 = a2m1->negated();
	FRowDsptr m2m2 = a2m2->negated();
	FRowDsptr m2m3 = a2m3->negated();
	auto aaaa = std::make_shared<std::vector<double>>(3, 0.0);
	FRowDsptr zero = std::make_shared<FullRow<double>>(3, 0.0);
	auto mat11 = std::make_shared<FullMatrix<double>>(ListFRD{ a2m1, m2m2, m2m3 });
	auto mat12 = std::make_shared<FullMatrix<double>>(ListFRD{ a2m2, a2m1, zero });
	auto mat13 = std::make_shared<FullMatrix<double>>(ListFRD{ a2m3, zero, a2m1 });
	auto mat14 = std::make_shared<FullMatrix<double>>(ListFRD{ zero, m2m3, a2m2 });
	auto mat22 = std::make_shared<FullMatrix<double>>(ListFRD{ m2m1, a2m2, m2m3 });
	auto mat23 = std::make_shared<FullMatrix<double>>(ListFRD{ zero, a2m3, a2m2 });
	auto mat24 = std::make_shared<FullMatrix<double>>(ListFRD{ a2m3, zero, m2m1 });
	auto mat33 = std::make_shared<FullMatrix<double>>(ListFRD{ m2m1, m2m2, a2m3 });
	auto mat34 = std::make_shared<FullMatrix<double>>(ListFRD{ m2m2, a2m1, zero });
	auto mat44 = std::make_shared<FullMatrix<double>>(ListFRD{ a2m1, a2m2, a2m3 });
	auto answer = std::make_unique<FullMatrix<std::shared_ptr<FullMatrix<double>>>>(4, 4);
	auto row1 = answer->at(0);
	row1->at(0) = mat11;
	row1->at(1) = mat12;
	row1->at(2) = mat13;
	row1->at(3) = mat14;
	auto row2 = answer->at(0);
	row2->at(0) = mat12;
	row2->at(1) = mat22;
	row2->at(2) = mat23;
	row2->at(3) = mat24;
	auto row3 = answer->at(0);
	row3->at(0) = mat13;
	row3->at(1) = mat23;
	row3->at(2) = mat33;
	row3->at(3) = mat34;
	auto row4 = answer->at(0);
	row4->at(0) = mat14;
	row4->at(1) = mat24;
	row4->at(2) = mat34;
	row4->at(3) = mat44;
	return answer;
}
